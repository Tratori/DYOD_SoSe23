#include "dictionary_segment.hpp"
#include "fixed_width_integer_vector.hpp"
#include "utils/assert.hpp"
#include "value_segment.hpp"

namespace opossum {

template <typename T>
DictionarySegment<T>::DictionarySegment(const std::shared_ptr<AbstractSegment>& abstract_segment) {
  // TODO: Split up into 2 methods, creating the dictionary and the attribute vector each. 
  
  // creating dictionary
  auto value_segment = std::dynamic_pointer_cast<ValueSegment<T>>(abstract_segment);
  std::vector<T> values = value_segment->values();
  std::sort(values.begin(), values.end());
  auto last_distinct = std::unique(values.begin(), values.end());
  values.erase(last_distinct, values.end());

  // If you have an idea how to avoid the following 7 lines, feel free
  auto contains_null = value_segment->is_nullable() && (std::find(value_segment->null_values().begin(), value_segment->null_values().end(), true) != value_segment->null_values().end()); 
  if(contains_null){
    values.erase(values.begin());
  }
  if(values.size() + contains_null + 1 > std::numeric_limits<ChunkOffset>::max()){
    Fail("Dictionary Segment cannot represent INVALID_VALUE_ID, NULL Values and all other values.");
  }

  _dictionary = values;
  
  // creating lookup
  std::unordered_map<T, ValueID> inverted_dictionary;
  u_int32_t dict_size = _dictionary.size(); 
  for(u_int32_t dict_key = 0; dict_key < dict_size; dict_key++){
    inverted_dictionary.insert({_dictionary[dict_key], ValueID{dict_key}});
  }

  auto vector_size = abstract_segment->size(); 
  // initialize attribute vector with size of current segment
  _attribute_vector = std::make_shared<FixedWidthIntegerVector>(vector_size);

  // filling attribute vector with index of values
  for(ChunkOffset val_index = 0; val_index < vector_size; val_index++){
    if(value_segment->null_values()[val_index]){
      _attribute_vector->set(val_index, null_value_id());
    } else {
      _attribute_vector->set(val_index, inverted_dictionary[values[val_index]]);
    }
  }
}

template <typename T>
AllTypeVariant DictionarySegment<T>::operator[](const ChunkOffset chunk_offset) const {
  // Implementation goes here
  // Fail("Implementation is missing.");
  return _dictionary[_attribute_vector->get(chunk_offset)];
}

template <typename T>
T DictionarySegment<T>::get(const ChunkOffset chunk_offset) const {
  // Implementation goes here
  // Fail("Implementation is missing.");
  return _dictionary[_attribute_vector->get(chunk_offset)];
}

template <typename T>
std::optional<T> DictionarySegment<T>::get_typed_value(const ChunkOffset chunk_offset) const {
  // Implementation goes here
  // Fail("Implementation is missing.");
  if(_attribute_vector->get(chunk_offset) == null_value_id()){
    return std::nullopt;
  }
  return _dictionary[_attribute_vector->get(chunk_offset)];
}

template <typename T>
const std::vector<T>& DictionarySegment<T>::dictionary() const {
  // Implementation goes here
  //Fail("Implementation is missing.");
  return _dictionary; 
}

template <typename T>
std::shared_ptr<const AbstractAttributeVector> DictionarySegment<T>::attribute_vector() const {
  // Implementation goes here
  // Fail("Implementation is missing.");
  return _attribute_vector; 
}

template <typename T>
ValueID DictionarySegment<T>::null_value_id() const {
  // Implementation goes here
  return static_cast<ValueID>(_dictionary.size()); 
  // Fail("Implementation is missing.");
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
  // Fail("Implementation is missing.");
  return _dictionary.size(); 
}

template <typename T>
ChunkOffset DictionarySegment<T>::size() const {
  // Implementation goes here
  return static_cast<ChunkOffset>(_attribute_vector->size());
}

template <typename T>
size_t DictionarySegment<T>::estimate_memory_usage() const {
  return size_t{};
}

EXPLICITLY_INSTANTIATE_DATA_TYPES(DictionarySegment);

}  // namespace opossum
