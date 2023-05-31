#include "base_test.hpp"

#include "storage/table.hpp"

namespace opossum {

class StorageTableTest : public BaseTest {
 protected:
  void SetUp() override {
    table.add_column("col_1", "int", false);
    table.add_column("col_2", "string", true);
  }

  Table table{2};
};

TEST_F(StorageTableTest, ChunkCount) {
  EXPECT_EQ(table.chunk_count(), 1);
  table.append({4, "Hello,"});
  table.append({6, "world"});
  table.append({3, "!"});
  EXPECT_EQ(table.chunk_count(), 2);
}

TEST_F(StorageTableTest, GetChunk) {
  table.get_chunk(ChunkID{0});
  table.append({4, "Hello,"});
  table.append({6, "world"});
  table.append({3, "!"});
  table.get_chunk(ChunkID{1});
  const auto chunk = table.get_chunk(ChunkID{0});
  EXPECT_THROW(table.get_chunk(ChunkID{7}), std::logic_error);
  EXPECT_EQ(chunk->size(), 2);
}

TEST_F(StorageTableTest, ColumnCount) {
  EXPECT_EQ(table.column_count(), 2);
}

TEST_F(StorageTableTest, RowCount) {
  EXPECT_EQ(table.row_count(), 0);
  table.append({4, "Hello,"});
  table.append({6, "world"});
  table.append({3, "!"});
  table.append({7, NULL_VALUE});
  EXPECT_EQ(table.row_count(), 4);
}

TEST_F(StorageTableTest, AddColumn) {
  EXPECT_EQ(table.column_count(), 2);
  table.add_column("col_3", "int", true);
  EXPECT_EQ(table.column_count(), 3);
  EXPECT_THROW(table.add_column("col_3", "int", false), std::logic_error);
  EXPECT_EQ(table.column_count(), 3);
}

TEST_F(StorageTableTest, GetColumnName) {
  EXPECT_EQ(table.column_name(ColumnID{0}), "col_1");
  EXPECT_EQ(table.column_name(ColumnID{1}), "col_2");
  EXPECT_EQ(table.column_names(), std::vector<std::string>({"col_1", "col_2"}));
  EXPECT_THROW(table.column_name(ColumnID{7}), std::logic_error);
}

TEST_F(StorageTableTest, GetColumnType) {
  EXPECT_EQ(table.column_type(ColumnID{0}), "int");
  EXPECT_EQ(table.column_type(ColumnID{1}), "string");
  EXPECT_THROW(table.column_type(ColumnID{7}), std::logic_error);
}

TEST_F(StorageTableTest, ColumnNullable) {
  EXPECT_FALSE(table.column_nullable(ColumnID{0}));
  EXPECT_TRUE(table.column_nullable(ColumnID{1}));
  EXPECT_THROW(table.column_nullable(ColumnID{7}), std::logic_error);
}

TEST_F(StorageTableTest, GetColumnIdByName) {
  EXPECT_EQ(table.column_id_by_name("col_2"), 1);
  EXPECT_THROW(table.column_id_by_name("no_column_name"), std::logic_error);
}

TEST_F(StorageTableTest, GetChunkSize) {
  EXPECT_EQ(table.target_chunk_size(), 2);
}

TEST_F(StorageTableTest, CreateNewChunk) {
  EXPECT_EQ(table.chunk_count(), 1);
  EXPECT_EQ(table.target_chunk_size(), 2);
  table.append({4, "Hello,"});
  table.append({6, "world"});
  table.append({3, "!"});
  EXPECT_EQ(table.chunk_count(), 2);
  table.append({4, "New Chunk is now allowed"});
  table.create_new_chunk();
  EXPECT_EQ(table.chunk_count(), 3);
}

TEST_F(StorageTableTest, AppendNullValues) {
  EXPECT_EQ(table.row_count(), 0);
  table.append({1, NULL_VALUE});
  EXPECT_EQ(table.row_count(), 1);
  EXPECT_THROW(table.append({NULL_VALUE, "foo"}), std::logic_error);
}

TEST_F(StorageTableTest, CompressChunkMultithreading) {
  const auto number_columns = ColumnID{100};
  const auto chunk_size = ChunkOffset{1000};

  const auto col_id_to_name = [](ColumnID col_id) { return "col_" + std::to_string(col_id); };

  auto large_table = Table{chunk_size};
  auto data_copy = std::vector<std::vector<AllTypeVariant>>();
  // Add columns.
  for (auto col_id = ColumnID{0}; col_id < number_columns; ++col_id) {
    large_table.add_column(col_id_to_name(col_id), "int", false);
  }

  // Add rows.
  for (auto row_id = ChunkOffset{0}; row_id < chunk_size; ++row_id) {
    auto row = std::vector<AllTypeVariant>();
    row.reserve(number_columns);
    for (auto col_id = ColumnID{0}; col_id < number_columns; ++col_id) {
      auto val = rand() % 100;  // NOLINT
      row.push_back(val);
    }
    large_table.append(row);
    data_copy.push_back(row);
  }

  large_table.compress_chunk(ChunkID{0});

  const auto column_names = large_table.column_names();
  const auto chunk = large_table.get_chunk(ChunkID{0});

  // Checking if compressed chunk and data copy are still equal.
  for (auto col_id = ColumnID{0}; col_id < number_columns; ++col_id) {
    auto compressed_column = chunk->get_segment(col_id);
    for (auto row_id = ChunkOffset{0}; row_id < chunk_size; ++row_id) {
      EXPECT_EQ(data_copy[row_id][col_id], (*compressed_column)[row_id]);
    }
  }
}

TEST_F(StorageTableTest, CompressChunk) {
  table.compress_chunk(ChunkID{0});
}

TEST_F(StorageTableTest, SegmentsNullable) {
  table.append({1, "foo"});
  ASSERT_EQ(table.chunk_count(), 1);
  const auto& chunk = table.get_chunk(ChunkID{0});
  ASSERT_TRUE(chunk);

  const auto& value_segment_1 = std::dynamic_pointer_cast<ValueSegment<int32_t>>(chunk->get_segment(ColumnID{0}));
  ASSERT_TRUE(value_segment_1);
  EXPECT_FALSE(value_segment_1->is_nullable());

  const auto& value_segment_2 = std::dynamic_pointer_cast<ValueSegment<std::string>>(chunk->get_segment(ColumnID{1}));
  ASSERT_TRUE(value_segment_2);
  EXPECT_TRUE(value_segment_2->is_nullable());
}

TEST_F(StorageTableTest, AppendWithEncodedSegments) {
  table.append({1, "foo"});
  EXPECT_EQ(table.row_count(), 1);

  table.compress_chunk(ChunkID{0});
  table.append({2, "bar"});

  EXPECT_EQ(table.row_count(), 2);
  EXPECT_EQ(table.chunk_count(), 2);
}

}  // namespace opossum
