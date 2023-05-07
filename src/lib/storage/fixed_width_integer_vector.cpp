#include "fixed_width_integer_vector.hpp"

namespace opossum {

FixedWidthIntegerVector::FixedWidthIntegerVector(size_t size) {}

ValueID FixedWidthIntegerVector::get(const size_t index) const {
  return ValueID{0};
}

// Sets the value id at a given position.
void FixedWidthIntegerVector::set(const size_t index, const ValueID value_id) {
  return;
}

// Returns the number of values.
size_t FixedWidthIntegerVector::size() const {
  return 0;
}

// Returns the width of biggest value id in bytes.
AttributeVectorWidth FixedWidthIntegerVector::width() const {
  return 0;
}

}  // namespace opossum
