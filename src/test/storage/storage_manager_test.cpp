#include "base_test.hpp"

#include "storage/storage_manager.hpp"

namespace opossum {

class StorageStorageManagerTest : public BaseTest {
 protected:
  void SetUp() override {
    auto& storage_manager = StorageManager::get();
    auto table_a = std::make_shared<Table>();
    auto table_b = std::make_shared<Table>(4);

    storage_manager.add_table("first_table", table_a);
    storage_manager.add_table("second_table", table_b);
  }
};

TEST_F(StorageStorageManagerTest, AddTable) {
  auto& storage_manager = StorageManager::get();
  auto table_c = std::make_shared<Table>();
  // Table already exists
  EXPECT_THROW(storage_manager.add_table("first_table", table_c), std::logic_error);
}

TEST_F(StorageStorageManagerTest, GetTable) {
  auto& storage_manager = StorageManager::get();
  auto table_c = storage_manager.get_table("first_table");
  auto table_d = storage_manager.get_table("second_table");
  EXPECT_THROW(storage_manager.get_table("third_table"), std::logic_error);
}

TEST_F(StorageStorageManagerTest, DropTable) {
  auto& storage_manager = StorageManager::get();
  storage_manager.drop_table("first_table");
  EXPECT_THROW(storage_manager.get_table("first_table"), std::logic_error);
  EXPECT_THROW(storage_manager.drop_table("first_table"), std::logic_error);
}

TEST_F(StorageStorageManagerTest, ResetTable) {
  StorageManager::get().reset();
  auto& storage_manager = StorageManager::get();
  EXPECT_THROW(storage_manager.get_table("first_table"), std::logic_error);
}

TEST_F(StorageStorageManagerTest, DoesNotHaveTable) {
  auto& storage_manager = StorageManager::get();
  EXPECT_EQ(storage_manager.has_table("third_table"), false);
}

TEST_F(StorageStorageManagerTest, HasTable) {
  auto& storage_manager = StorageManager::get();
  EXPECT_EQ(storage_manager.has_table("first_table"), true);
}

TEST_F(StorageStorageManagerTest, PrintTable) {
  auto& storage_manager = StorageManager::get();
  std::stringstream ss;
  storage_manager.print(ss);
  EXPECT_NE(ss.str(), "");

  ss.str("");
  ss.clear();
  storage_manager.reset();

  storage_manager.print(ss);
  EXPECT_EQ(ss.str(), "");
}

TEST_F(StorageStorageManagerTest, TableNames) {
  auto& storage_manager = StorageManager::get();
  auto table_names = storage_manager.table_names();

  EXPECT_TRUE(std::find(table_names.begin(), table_names.end(), "first_table") != table_names.end());
  EXPECT_TRUE(std::find(table_names.begin(), table_names.end(), "second_table") != table_names.end());

  storage_manager.reset();

  table_names = storage_manager.table_names();
  EXPECT_TRUE(std::find(table_names.begin(), table_names.end(), "first_table") == table_names.end());
}

}  // namespace opossum
