#include "table_scan.hpp"
#include "get_table.hpp"
#include "resolve_type.hpp"
#include "storage/dictionary_segment.hpp"
#include "storage/reference_segment.hpp"
#include "storage/table.hpp"
#include "storage/value_segment.hpp"
#include "types.hpp"

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
  const auto input_table = _left_input_table();

  // TODO(Robert): What if last operator does not return anything?
  Assert(input_table, "Performing a table scan without input does not work.");
  const auto chunk_count = input_table->chunk_count();
  auto const column_type = input_table->column_type(_column_id);
  auto result_table = Table{input_table->target_chunk_size()};

  resolve_data_type(column_type, [&](auto type) {
    using Type = typename decltype(type)::type;
    for (auto chunk_id = ChunkID{0}; chunk_id < chunk_count; ++chunk_id) {
      auto position_list = PosList();
      const auto chunk = input_table->get_chunk(chunk_id);
      const auto segment = chunk->get_segment(_column_id);

      const auto value_segment = std::dynamic_pointer_cast<ValueSegment<Type>>(segment);
      const auto dictionary_segment = std::dynamic_pointer_cast<DictionarySegment<Type>>(segment);
      const auto reference_segment = std::dynamic_pointer_cast<ReferenceSegment>(segment);

      std::function<bool(Type & a, Type & b)> op = [](Type& a, Type& b) { return true; };

      switch (_scan_type) {
        case ScanType::OpEquals:
          op = [](Type& a, Type& b) { return a == b; };
        case ScanType::OpNotEquals:
          op = [](Type& a, Type& b) { return a != b; };
        case ScanType::OpLessThan:
          op = [](Type& a, Type& b) { return a < b; };
        case ScanType::OpLessThanEquals:
          op = [](Type& a, Type& b) { return a <= b; };
        case ScanType::OpGreaterThan:
          op = [](Type& a, Type& b) { return a > b; };
        case ScanType::OpGreaterThanEquals:
          op = [](Type& a, Type& b) { return a >= b; };

        default:
          Fail("Scan type not defined.");
      }

      if (value_segment) {
        const auto values = value_segment->values();
        auto row_id = ChunkOffset{0};
        for (const auto& value : values) {
          if (op(value, _search_value)) {
            position_list.push_back(RowID{chunk_id, row_id});
          }
          row_id++;
        }
      } else if (dictionary_segment) {
      }
      //auto result_chunk = ReferenceSegment(input_table, _column_id, );
    }
  });

  //auto scan_type_function(ScanType scan_type) {
  //  switch (scan_type) {
  //    case ScanType::OpEquals:
  //      return [](auto& a, auto& b) { return a == b; };
  //    case ScanType::OpNotEquals:
  //      return [](auto& a, auto& b) { return a != b; };
  //    case ScanType::OpLessThan:
  //      return [](auto& a, auto& b) { return a < b; };
  //    case ScanType::OpLessThanEquals:
  //      return [](auto& a, auto& b) { return a <= b; };
  //    case ScanType::OpGreaterThan:
  //      return [](auto& a, auto& b) { return a > b; };
  //    case ScanType::OpGreaterThanEquals:
  //      return [](auto& a, auto& b) { return a >= b; };
  //
  //    default:
  //      Fail("Scan type not defined.");
  //  }}

  return nullptr;
}
};  // namespace opossum
