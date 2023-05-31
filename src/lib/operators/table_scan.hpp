#pragma once

#include "abstract_operator.hpp"
#include "all_type_variant.hpp"
#include "utils/assert.hpp"
#include "storage/dictionary_segment.hpp"
#include "storage/reference_segment.hpp"

namespace opossum {

class TableScan : public AbstractOperator {
 public:
  TableScan(const std::shared_ptr<const AbstractOperator>& in, const ColumnID column_id, const ScanType scan_type,
            const AllTypeVariant search_value);

  ColumnID column_id() const;

  ScanType scan_type() const;

  const AllTypeVariant& search_value() const;

 protected:
  template <typename T>
  std::function<bool(T, T)> _create_scan_operation() const;

  template <typename T>
  std::shared_ptr<PosList> _tablescan_dict_segment(std::shared_ptr<DictionarySegment<T>> segment, ChunkID chunk_id);
  template <typename T>
  std::shared_ptr<PosList> _tablescan_value_segment(std::shared_ptr<ValueSegment<T>> segment, ChunkID chunk_id);
  template <typename T>
  std::shared_ptr<PosList> _tablescan_reference_segment(std::shared_ptr<ReferenceSegment> segment, ChunkID chunk_id);

  std::shared_ptr<const Table> _on_execute() override;
  ColumnID _column_id;
  ScanType _scan_type;
  AllTypeVariant _search_value;
};

}  // namespace opossum
