#include "storage_manager.hpp"

#include "utils/assert.hpp"

namespace opossum {

StorageManager& StorageManager::get() {
  return *(new StorageManager());
  // A really hacky fix to get the tests to run - replace this with your implementation
}

void StorageManager::add_table(const std::string& name, std::shared_ptr<Table> table) {
  // Implementation goes here
  Fail("Implementation is missing.");
}

void StorageManager::drop_table(const std::string& name) {
  // Implementation goes here
  Fail("Implementation is missing.");
}

std::shared_ptr<Table> StorageManager::get_table(const std::string& name) const {
  // Implementation goes here
  Fail("Implementation is missing.");
}

bool StorageManager::has_table(const std::string& name) const {
  // Implementation goes here
  Fail("Implementation is missing.");
}

std::vector<std::string> StorageManager::table_names() const {
  // Implementation goes here
  Fail("Implementation is missing.");
}

void StorageManager::print(std::ostream& out) const {
  // Implementation goes here
  Fail("Implementation is missing.");
}

void StorageManager::reset() {
  // Implementation goes here
}

}  // namespace opossum
