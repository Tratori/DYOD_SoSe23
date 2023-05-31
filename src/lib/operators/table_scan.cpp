#include "table_scan.hpp"
#include "get_table.hpp"

namespace opossum {

TableScan::TableScan(const std::shared_ptr<const AbstractOperator>& in, const ColumnID column_id,
                     const ScanType scan_type, const AllTypeVariant search_value)
    : _column_id{column_id}, _scan_type{scan_type}, _search_value{search_value} {}

ColumnID TableScan::column_id() const {
  return _column_id;
}

ScanType TableScan::scan_type() const {
  return _scan_type;
}

const AllTypeVariant& TableScan::search_value() const {
  return _search_value;
}

std::shared_ptr<const Table> TableScan::_on_execute() {
  const auto input = _left_input->get_output();
  const auto comparitor = _search_value;

  // TODO(Robert): What if last operator does not return anything?
  Assert(input, "Performing a table scan without input does not work.");

  auto comp_op = []() {};
}
};  // namespace opossum
