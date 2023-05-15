#include <thread>

#include "dictionary_segment.hpp"
#include "resolve_type.hpp"
#include "table.hpp"
#include "utils/assert.hpp"
namespace opossum {

Table::Table(const ChunkOffset target_chunk_size) {
  _target_chunk_size = target_chunk_size;
  _chunks = std::vector<std::shared_ptr<Chunk>>{std::make_shared<Chunk>()};
}

void Table::add_column_definition(const std::string& name, const std::string& type, const bool nullable) {
  DebugAssert(_column_names.size() == _column_types.size() && _column_types.size() == _column_nullable.size(),
              "Columns are not well defined");
  if (std::find(_column_names.begin(), _column_names.end(), name) != _column_names.end()) {
    throw std::logic_error("It is not allowed to add two columns with the same name.");
  }

  _column_names.emplace_back(name);
  _column_types.emplace_back(type);
  _column_nullable.emplace_back(nullable);
}

void Table::add_column(const std::string& name, const std::string& type, const bool nullable) {
  DebugAssert(_chunks[0]->size() == 0, "Adding columns is only allowed if the table does not have any entries.");
  add_column_definition(name, type, nullable);
  resolve_data_type(type, [&](const auto data_type_t) {
    using ColumnDataType = typename decltype(data_type_t)::type;
    const auto value_segment = std::make_shared<ValueSegment<ColumnDataType>>(nullable);
    _chunks[0]->add_segment(value_segment);
  });
}

void Table::create_new_chunk() {
  _chunks.emplace_back(std::make_shared<Chunk>());

  size_t num_columns = _column_types.size();
  for (unsigned int col_id = 0; col_id < num_columns; ++col_id) {
    resolve_data_type(_column_types[col_id], [&](const auto data_type_t) {
      using ColumnDataType = typename decltype(data_type_t)::type;
      const auto value_segment = std::make_shared<ValueSegment<ColumnDataType>>(_column_nullable[col_id]);
      _chunks.back()->add_segment(value_segment);
    });
  }
}

void Table::append(const std::vector<AllTypeVariant>& values) {
  auto is_dictionary_encoded = false;
  resolve_data_type(_column_types[0], [this, &is_dictionary_encoded](const auto data_type_t) {
    using ColumnDataType = typename decltype(data_type_t)::type;
    auto segment = _chunks.back()->get_segment(ColumnID{0});
    is_dictionary_encoded = static_cast<bool>(std::dynamic_pointer_cast<DictionarySegment<ColumnDataType>>(segment));
  });

  if (_chunks.back()->size() == _target_chunk_size || is_dictionary_encoded) {
    create_new_chunk();
  }
  _chunks.back()->append(values);
}

ColumnCount Table::column_count() const {
  return static_cast<ColumnCount>(_column_names.size());
}

uint64_t Table::row_count() const {
  // Chunks are not always full.
  auto row_count = uint64_t{0};
  for (auto chunk : _chunks) {
    row_count += chunk->size();
  }
  return row_count;
}

ChunkID Table::chunk_count() const {
  return static_cast<ChunkID>(_chunks.size());
}

ColumnID Table::column_id_by_name(const std::string& column_name) const {
  size_t num_columns = _column_names.size();
  for (uint16_t col_id = 0; col_id < num_columns; ++col_id) {
    if (_column_names[col_id] == column_name) {
      return ColumnID{col_id};
    }
  }
  throw std::logic_error("Table does not contain column with the requested name.");
}

ChunkOffset Table::target_chunk_size() const {
  return _target_chunk_size;
}

const std::vector<std::string>& Table::column_names() const {
  return _column_names;
}

const std::string& Table::column_name(const ColumnID column_id) const {
  if (column_id > _column_names.size()) {
    throw std::logic_error("Table does not contain column with the requested id.");
  }
  return _column_names[column_id];
}

const std::string& Table::column_type(const ColumnID column_id) const {
  if (column_id > _column_types.size()) {
    throw std::logic_error("Table does not contain column with the requested id.");
  }
  return _column_types[column_id];
}

bool Table::column_nullable(const ColumnID column_id) const {
  if (column_id > _column_nullable.size()) {
    throw std::logic_error("Table does not contain column with the requested id.");
  }
  return _column_nullable[column_id];
}

std::shared_ptr<Chunk> Table::get_chunk(ChunkID chunk_id) {
  if (chunk_id >= static_cast<ChunkID>(_chunks.size())) {
    throw std::logic_error("Table does not contain chunk with the requested id.");
  }
  return _chunks[chunk_id];
}

std::shared_ptr<const Chunk> Table::get_chunk(ChunkID chunk_id) const {
  if (chunk_id >= static_cast<ChunkID>(_chunks.size())) {
    throw std::logic_error("Table does not contain chunk with the requested id.");
  }
  return _chunks[chunk_id];
}

void Table::compress_chunk(const ChunkID chunk_id) {
  auto compressed_chunk = std::make_shared<Chunk>();
  auto threads = std::vector<std::thread>(column_count());

  const auto compression_functor = [&](ColumnID column_id) {
    static std::mutex mutex;
    resolve_data_type(_column_types[column_id], [&](const auto data_type_t) {
      using ColumnDataType = typename decltype(data_type_t)::type;

      auto value_segment = get_chunk(chunk_id)->get_segment(column_id);
      auto dictionary_segment = std::make_shared<DictionarySegment<ColumnDataType>>(value_segment);

      std::lock_guard<std::mutex> lock(mutex);
      compressed_chunk->add_segment(dictionary_segment);
    });
  };

  for (auto col_id = ColumnID{0}; col_id < column_count(); col_id++) {
    threads[col_id] = std::thread(compression_functor, col_id);
  }

  for (auto thread_index = uint32_t{0}; thread_index < threads.size(); ++thread_index) {
    threads[thread_index].join();
  }
  _chunks[chunk_id] = compressed_chunk;
}

}  // namespace opossum
