#pragma once

#include "abstract_attribute_vector.hpp"
#include "abstract_segment.hpp"
#include "types.hpp"
#include "vector"

namespace opossum {

template <typename T>
class FixedWidthIntegerVector : public AbstractAttributeVector {
 public:
  explicit FixedWidthIntegerVector(size_t size);

  ValueID get(const size_t index) const;

  // Sets the value id at a given position.
  void set(const size_t index, const ValueID value_id);

  // Returns the number of values.
  size_t size() const;

  // Returns the width of biggest value id in bytes.
  AttributeVectorWidth width() const;

 protected:
  std::vector<T> _attribute_vector;
};

}  // namespace opossum
