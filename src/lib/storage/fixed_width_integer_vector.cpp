#include "fixed_width_integer_vector.hpp"

namespace opossum {

FixedWidthIntegerVector::FixedWidthIntegerVector(size_t size) {
  // TODO: what should be our default value here? 
  // do we want to store a Vector of ValueIDs instead of u_int32_t? 
  _attribute_vector = std::vector<u_int32_t>(size, -1); 
}

ValueID FixedWidthIntegerVector::get(const size_t index) const {
  if(index >= _attribute_vector.size()){
    throw std::logic_error("Index out of bounds"); 
  }
  return ValueID{_attribute_vector[index]};
}

// Sets the value id at a given position.
void FixedWidthIntegerVector::set(const size_t index, const ValueID value_id) {
  // Do we really want this check here or would DebugAssert be enough? (Same for the getter)
  if(index >= _attribute_vector.size()){
    throw std::logic_error("Index out of bounds"); 
  }
  _attribute_vector[index] = value_id; 
}

// Returns the number of values.
size_t FixedWidthIntegerVector::size() const {
  return _attribute_vector.size();
}

// Returns the width of biggest value id in bytes.
AttributeVectorWidth FixedWidthIntegerVector::width() const {
  return AttributeVectorWidth{4};
}

}  // namespace opossum
