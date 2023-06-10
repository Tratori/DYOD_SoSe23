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

  Assert(input_table, "Performing a table scan without input does not work.");
  const auto chunk_count = input_table->chunk_count();
  const auto column_type = input_table->column_type(_column_id);
  auto output_reference_segments = std::vector<std::shared_ptr<ReferenceSegment>>{};

  // any comparison with null will always return an empty set
  if (variant_is_null(_search_value)) {
    return std::make_shared<Table>(*input_table, output_reference_segments);
  }

  // We might end up using far less segments, but still should be worth to reserve the space for worst case.
  output_reference_segments.reserve(chunk_count);

  resolve_data_type(column_type, [&](auto type) {
    using Type = typename decltype(type)::type;

    for (auto chunk_id = ChunkID{0}; chunk_id < chunk_count; ++chunk_id) {
      const auto chunk = input_table->get_chunk(chunk_id);
      // if chunk is empty, skip this chunk
      if (!chunk->size()) {
        continue;
      }

      auto position_list = std::make_shared<PosList>();
      const auto segment = chunk->get_segment(_column_id);

      const auto value_segment = std::dynamic_pointer_cast<ValueSegment<Type>>(segment);
      const auto dictionary_segment = std::dynamic_pointer_cast<DictionarySegment<Type>>(segment);
      const auto reference_segment = std::dynamic_pointer_cast<ReferenceSegment>(segment);

      Assert(value_segment || dictionary_segment || reference_segment,
             "TableScan was called on unsupported segment type");

      if (value_segment) {
        position_list = _tablescan_value_segment<Type>(value_segment, chunk_id);
      } else if (dictionary_segment) {
        position_list = _tablescan_dict_segment<Type>(dictionary_segment, chunk_id);
      } else if (reference_segment) {
        position_list = _tablescan_reference_segment<Type>(reference_segment, chunk_id);
      }

      if (!position_list->empty()) {
        // Only keep non empty segments and change referred table, if we scanned a ReferenceSegment.
        output_reference_segments.push_back(std::make_shared<ReferenceSegment>(
            (reference_segment) ? reference_segment->referenced_table() : input_table, _column_id, position_list));
      }
    }
  });

  return std::make_shared<Table>(*input_table, output_reference_segments);
}

template <typename T>
std::shared_ptr<PosList> TableScan::_tablescan_dict_segment(std::shared_ptr<DictionarySegment<T>> segment,
                                                            ChunkID chunk_id) {
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
     * As we are gonna compare with the valueID and not the actual value, some other
     * operators also need to be overwritten, to work accordingly.
     */
    case ScanType::OpEquals:
      if (lower_bound == upper_bound) {
        return position_list;
      }
      break;
    case ScanType::OpNotEquals:
      if (lower_bound == upper_bound)
        scan_op = [](auto l, auto r) { return true; };
      break;
    case ScanType::OpLessThanEquals:
      scan_op = [&](auto l, auto r) { return l < upper_bound; };
      break;
    case ScanType::OpGreaterThan:
      scan_op = [&](auto l, auto r) { return l >= upper_bound; };
      break;
      // Rest stays as it is.
    default:
      break;
  }

  const auto attr_vector = segment->attribute_vector();
  for (auto index = ChunkOffset{0}; index < segment->size(); ++index) {
    const auto value_id = attr_vector->get(index);
    if (value_id != segment->null_value_id() && scan_op(value_id, lower_bound)) {
      position_list->push_back(RowID{chunk_id, index});
    }
  }

  return position_list;
}

template <typename T>
std::shared_ptr<PosList> TableScan::_tablescan_value_segment(std::shared_ptr<ValueSegment<T>> segment,
                                                             ChunkID chunk_id) {
  const auto scan_op = _create_scan_operation<T>();
  const auto values = segment->values();

  auto position_list = std::make_shared<PosList>();
  const auto search_val = type_cast<T>(_search_value);

  auto index = ChunkOffset{0};
  for (const auto& value : values) {
    if (!segment->is_null(index) && scan_op(value, search_val)) {
      position_list->push_back(RowID{chunk_id, index});
    }
    ++index;
  }

  return position_list;
}

template <typename T>
std::shared_ptr<PosList> TableScan::_tablescan_reference_segment(std::shared_ptr<ReferenceSegment> segment,
                                                                 ChunkID chunk_id) {
  auto position_list = std::make_shared<PosList>();

  const auto input_position_list = segment->pos_list();
  const auto table = segment->referenced_table();
  const auto search_val = type_cast<T>(search_value());
  const auto scan_op = _create_scan_operation<T>();

  for (auto index = ChunkOffset{0}; index < segment->size(); ++index) {
    const auto row = (*input_position_list)[index];
    const auto chunk = table->get_chunk(row.chunk_id);
    const auto target_segment = chunk->get_segment(_column_id);

    const auto dict_segment = std::dynamic_pointer_cast<DictionarySegment<T>>(target_segment);
    const auto val_segment = std::dynamic_pointer_cast<ValueSegment<T>>(target_segment);

    Assert(dict_segment || val_segment, "Segment that ReferenceSegement references is not supported by TableScan.");

    if (val_segment) {
      // If value is null, it cannot appear in result set, so just continue.
      if (val_segment->is_null(row.chunk_offset)) {
        continue;
      }
      const auto values = val_segment->values();
      const auto casted_value = type_cast<T>(values[row.chunk_offset]);

      if (scan_op(casted_value, search_val)) {
        position_list->emplace_back((*input_position_list)[index]);
      }
    } else if (dict_segment) {
      const auto& attribute_vector = dict_segment->attribute_vector();
      const auto value_id = attribute_vector->get(row.chunk_offset);
      // If value is null, it cannot appear in result set, so just continue.
      if (value_id == dict_segment->null_value_id()) {
        continue;
      }
      const auto value = dict_segment->value_of_value_id(value_id);
      const auto casted_value = type_cast<T>(value);

      if (scan_op(casted_value, search_val)) {
        position_list->emplace_back((*input_position_list)[index]);
      }
    }
  }

  return position_list;
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
