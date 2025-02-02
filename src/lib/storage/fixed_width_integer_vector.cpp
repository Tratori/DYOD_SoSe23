#include "fixed_width_integer_vector.hpp"
#include "all_type_variant.hpp"
#include "utils/assert.hpp"

namespace opossum {

template <typename T>
FixedWidthIntegerVector<T>::FixedWidthIntegerVector(size_t size) {
  _attribute_vector = std::vector<T>(size);
}

template <typename T>
ValueID FixedWidthIntegerVector<T>::get(const size_t index) const {
  DebugAssert(index < _attribute_vector.size(), "Index out of bounds");
  return ValueID{_attribute_vector[index]};
}

// Sets the value id at a given position.
template <typename T>
void FixedWidthIntegerVector<T>::set(const size_t index, const ValueID value_id) {
  DebugAssert(index < _attribute_vector.size(), "Index out of bounds");
  _attribute_vector[index] = value_id;
}

// Returns the number of values.
template <typename T>
size_t FixedWidthIntegerVector<T>::size() const {
  return _attribute_vector.size();
}

// Returns the width of biggest value id in bytes.
template <typename T>
AttributeVectorWidth FixedWidthIntegerVector<T>::width() const {
  return AttributeVectorWidth{sizeof(T)};
}

template class FixedWidthIntegerVector<u_int8_t>;
template class FixedWidthIntegerVector<u_int16_t>;
template class FixedWidthIntegerVector<u_int32_t>;
}  // namespace opossum
