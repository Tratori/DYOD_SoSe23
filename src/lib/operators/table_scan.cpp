#include "table_scan.hpp"
#include "get_table.hpp"
#include "resolve_type.hpp"
#include "storage/abstract_attribute_vector.hpp"
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
      // if chunk is empty, skip this chunk
      if (!chunk->size()) {
        continue;
      }
      const auto segment = chunk->get_segment(_column_id);

      const auto value_segment = std::dynamic_pointer_cast<ValueSegment<Type>>(segment);
      const auto dictionary_segment = std::dynamic_pointer_cast<DictionarySegment<Type>>(segment);
      const auto reference_segment = std::dynamic_pointer_cast<ReferenceSegment>(segment);

      if (value_segment) {
        position_list = _tablescan_value_segment<Type>(value_segment, chunk_id);
      } else if (dictionary_segment) {
        position_list = _tablescan_dict_segment<Type>(dictionary_segment, chunk_id);
      } else if (reference_segment) {
        position_list = _tablescan_reference_segment<Type>(reference_segment, chunk_id);
      }
    }
  });

  return std::make_shared<Table>(*input_table, output_reference_segments);
}

template <typename T>
std::shared_ptr<PosList> TableScan::_tablescan_dict_segment(std::shared_ptr<DictionarySegment<T>> segment, ChunkID chunk_id) {
  auto position_list = std::make_shared<PosList>();
  const auto search_val = type_cast<T>(_search_value);

  // Not comparing the individual types anymore, only indexes now.
  auto scan_op = _create_scan_operation<ValueID>();

  const auto lower_bound = segment->lower_bound(search_val);
  const auto upper_bound = segment->upper_bound(search_val);

  switch (_scan_type) {
    /**
     * Upper Bound is exclusive ==> [minElement, upperBound)
     * Lower Bound is inclusive ==> [lowerBound, maxElement]
     * 
     * Therefore, if they match, there is no matching element with the search value.
     */ 
    case ScanType::OpEquals:
      if (lower_bound == upper_bound) scan_op = [](auto l, auto r) { return false; };
      break;
    case ScanType::OpNotEquals:
      if (lower_bound == upper_bound) scan_op = [](auto l, auto r) { return true; };
      break;
    // Rest stays as it is.
    case ScanType::OpLessThan:
      break;
    case ScanType::OpLessThanEquals:
      break;
    case ScanType::OpGreaterThan:
      break;
    case ScanType::OpGreaterThanEquals:
      break;
  }

  const auto attr_vector = segment->attribute_vector();
  for (auto value_ind = ChunkOffset{0}; value_ind < segment->size(); ++value_ind) {
    if (scan_op(attr_vector->get(value_ind), lower_bound)) {
      position_list->push_back(RowID{chunk_id, value_ind});
    }
  }

  return position_list;
}

template <typename T>
std::shared_ptr<PosList> TableScan::_tablescan_value_segment(std::shared_ptr<ValueSegment<T>> segment, ChunkID chunk_id) {
  const auto scan_op = _create_scan_operation<T>();
  const auto values = segment->values();

  auto position_list = std::make_shared<PosList>();
  const auto search_val = type_cast<T>(_search_value);

  for (const auto& value : values) {
    if (scan_op(value, search_val)) {
      position_list->push_back(RowID{chunk_id, _column_id});
    }
  }

  return position_list;
}

template <typename T>
std::shared_ptr<PosList> TableScan::_tablescan_reference_segment(std::shared_ptr<ReferenceSegment> segment, ChunkID chunk_id) {
  auto position_list = std::make_shared<PosList>();

  return nullptr;
}


template <typename T>
std::function<bool(T, T)> TableScan::_create_scan_operation() const {
  switch (_scan_type) {
    case ScanType::OpEquals:
      return [](auto l, auto r) { return l == r; };

    case ScanType::OpNotEquals:
      return [](auto l, auto r) { return l != r; };

    case ScanType::OpLessThan:
      return [](auto l, auto r) { return l < r; };

    case ScanType::OpLessThanEquals:
      return [](auto l, auto r) { return l <= r; };

    case ScanType::OpGreaterThan:
      return [](auto l, auto r) { return l > r; };

    case ScanType::OpGreaterThanEquals:
      return [](auto l, auto r) { return l >= r; };

    default:
      Fail("Scan Operation not available.");
      break;
  }
}

};  // namespace opossum
