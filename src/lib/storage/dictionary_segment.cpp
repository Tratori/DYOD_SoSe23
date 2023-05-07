#include "dictionary_segment.hpp"
#include "fixed_width_integer_vector.hpp"
#include "utils/assert.hpp"
#include "value_segment.hpp"

namespace opossum {

template <typename T>
DictionarySegment<T>::DictionarySegment(const std::shared_ptr<AbstractSegment>& abstract_segment) {
  // creating dictionary
  auto value_segment = std::dynamic_pointer_cast<ValueSegment<T>>(abstract_segment);
  std::vector<T> values = value_segment->values();
  std::sort(values.begin(), values.end());
  auto last_distinct = std::unique(values.begin(), values.end());
  values.erase(last_distinct, values.end());
  _dictionary = values;

  // creating lookup
  std::unordered_map<T, ValueID> inverted_dictionary;
  u_int32_t dict_size = _dictionary.size(); 
  for(u_int32_t dict_key = 0; dict_key < dict_size; dict_key++){
    inverted_dictionary.insert({_dictionary[dict_key], ValueID{dict_key}});
  }

  // initialize attribute vector with size of current segment
  _attribute_vector = std::make_shared<FixedWidthIntegerVector>(abstract_segment->size());
  // filling attribute vector with index of values
  int values_size = values.size(); 
  for(int val_index = 0; val_index < values_size; val_index++){
    _attribute_vector->set(val_index, inverted_dictionary[values[val_index]]);
  }
}

template <typename T>
AllTypeVariant DictionarySegment<T>::operator[](const ChunkOffset chunk_offset) const {
  // Implementation goes here
  Fail("Implementation is missing.");
}

template <typename T>
T DictionarySegment<T>::get(const ChunkOffset chunk_offset) const {
  // Implementation goes here
  Fail("Implementation is missing.");
}

template <typename T>
std::optional<T> DictionarySegment<T>::get_typed_value(const ChunkOffset chunk_offset) const {
  // Implementation goes here
  Fail("Implementation is missing.");
}

template <typename T>
const std::vector<T>& DictionarySegment<T>::dictionary() const {
  // Implementation goes here
  Fail("Implementation is missing.");
}

template <typename T>
std::shared_ptr<const AbstractAttributeVector> DictionarySegment<T>::attribute_vector() const {
  // Implementation goes here
  Fail("Implementation is missing.");
}

template <typename T>
ValueID DictionarySegment<T>::null_value_id() const {
  // Implementation goes here
  Fail("Implementation is missing.");
}

template <typename T>
const T DictionarySegment<T>::value_of_value_id(const ValueID value_id) const {
  // Implementation goes here
  Fail("Implementation is missing.");
}

template <typename T>
ValueID DictionarySegment<T>::lower_bound(const T value) const {
  // Implementation goes here
  Fail("Implementation is missing.");
}

template <typename T>
ValueID DictionarySegment<T>::lower_bound(const AllTypeVariant& value) const {
  // Implementation goes here
  Fail("Implementation is missing.");
}

template <typename T>
ValueID DictionarySegment<T>::upper_bound(const T value) const {
  // Implementation goes here
  Fail("Implementation is missing.");
}

template <typename T>
ValueID DictionarySegment<T>::upper_bound(const AllTypeVariant& value) const {
  // Implementation goes here
  Fail("Implementation is missing.");
}

template <typename T>
ChunkOffset DictionarySegment<T>::unique_values_count() const {
  // Implementation goes here
  Fail("Implementation is missing.");
}

template <typename T>
ChunkOffset DictionarySegment<T>::size() const {
  // Implementation goes here
  return ChunkOffset{};
}

template <typename T>
size_t DictionarySegment<T>::estimate_memory_usage() const {
  return size_t{};
}

EXPLICITLY_INSTANTIATE_DATA_TYPES(DictionarySegment);

}  // namespace opossum
