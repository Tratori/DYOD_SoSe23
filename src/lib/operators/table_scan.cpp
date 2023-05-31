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
    : AbstractOperator{in}, _column_id{column_id}, _scan_type{scan_type}, _search_value{search_value} {}

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
  auto output_reference_segments = std::vector<std::shared_ptr<ReferenceSegment>>{};
  output_reference_segments.reserve(chunk_count);

  resolve_data_type(column_type, [&](auto type) {
    using Type = typename decltype(type)::type;

    for (auto chunk_id = ChunkID{0}; chunk_id < chunk_count; ++chunk_id) {
      auto position_list = std::make_shared<PosList>();
      const auto chunk = input_table->get_chunk(chunk_id);
      const auto segment = chunk->get_segment(_column_id);

      const auto value_segment = std::dynamic_pointer_cast<ValueSegment<Type>>(segment);
      const auto dictionary_segment = std::dynamic_pointer_cast<DictionarySegment<Type>>(segment);
      const auto reference_segment = std::dynamic_pointer_cast<ReferenceSegment>(segment);

      if (value_segment) {
        const auto values = value_segment->values();
        //auto row_id = ChunkOffset{0};
        for (const auto& value : values) {
          const auto scan_op = _create_scan_operation<Type>();
          const auto search_val = type_cast<Type>(_search_value);

          if (scan_op(value, search_val)) {
            position_list->push_back(RowID{chunk_id, _column_id});
          }
        }

        if (!position_list->empty()) {
          output_reference_segments.push_back(
              std::make_shared<ReferenceSegment>(input_table, _column_id, position_list));
        }
      } else if (dictionary_segment) {
      }
    }
  });

  return std::make_shared<Table>(*input_table, output_reference_segments);
}

template <typename T>
std::function<bool(T, T)> TableScan::_create_scan_operation() const {
  switch (scan_type()) {
    case ScanType::OpEquals:
      return [](auto left_operand, auto right_operand) { return left_operand == right_operand; };

    case ScanType::OpNotEquals:
      return [](auto left_operand, auto right_operand) { return left_operand != right_operand; };

    case ScanType::OpLessThan:
      return [](auto left_operand, auto right_operand) { return left_operand < right_operand; };

    case ScanType::OpLessThanEquals:
      return [](auto left_operand, auto right_operand) { return left_operand <= right_operand; };

    case ScanType::OpGreaterThan:
      return [](auto left_operand, auto right_operand) { return left_operand > right_operand; };

    case ScanType::OpGreaterThanEquals:
      return [](auto left_operand, auto right_operand) { return left_operand >= right_operand; };

    default:
      Fail("Invalid Scan Operation.");
      break;
  }
}

};  // namespace opossum
