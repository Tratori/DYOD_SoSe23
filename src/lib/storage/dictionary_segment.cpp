#include "dictionary_segment.hpp"
#include "fixed_width_integer_vector.hpp"
#include "type_cast.hpp"
#include "utils/assert.hpp"
#include "value_segment.hpp"

namespace opossum {

template <typename T>
DictionarySegment<T>::DictionarySegment(const std::shared_ptr<AbstractSegment>& abstract_segment) {
  // TODO(Paul): Split up into 2 methods, creating the dictionary and the attribute vector each.

  // creating dictionary
  auto value_segment = std::dynamic_pointer_cast<ValueSegment<T>>(abstract_segment);

  DebugAssert(value_segment, "Given segment is not a value segment.");
  auto values = value_segment->values();
  auto values_copy = std::vector<T>(values);

  std::sort(values.begin(), values.end());
  auto last_distinct = std::unique(values.begin(), values.end());
  values.erase(last_distinct, values.end());

  // If you have an idea how to avoid the following 7 lines, feel free
  auto contains_null = value_segment->is_nullable() &&
                       (std::find(value_segment->null_values().begin(), value_segment->null_values().end(), true) !=
                        value_segment->null_values().end());
  if (contains_null) {
    values.erase(values.begin());
  }
  if (values.size() + contains_null + 1 > std::numeric_limits<ChunkOffset>::max()) {
    Fail("Dictionary Segment cannot represent INVALID_VALUE_ID, NULL Values and all other values.");
  }

  _dictionary = values;

  // creating lookup
  std::unordered_map<T, ValueID> inverted_dictionary;
  const auto dict_size = _dictionary.size();
  for (auto dict_key = uint32_t{0}; dict_key < dict_size; dict_key++) {
    inverted_dictionary.insert({_dictionary[dict_key], ValueID{dict_key}});
  }

  const auto vector_size = abstract_segment->size();
  // -1 because we count from 0.
  if (dict_size - 1 <= std::numeric_limits<u_int8_t>::max()) {
    _attribute_vector = std::make_shared<FixedWidthIntegerVector<u_int8_t>>(vector_size);
  } else if (dict_size - 1 <= std::numeric_limits<u_int16_t>::max()) {
    _attribute_vector = std::make_shared<FixedWidthIntegerVector<u_int16_t>>(vector_size);
  } else if (dict_size - 1 <= std::numeric_limits<u_int32_t>::max()) {
    _attribute_vector = std::make_shared<FixedWidthIntegerVector<u_int32_t>>(vector_size);
  }

  // filling attribute vector with index of values
  for (auto val_index = ChunkOffset{0}; val_index < vector_size; val_index++) {
    if (value_segment->is_nullable() && value_segment->null_values()[val_index]) {
      _attribute_vector->set(val_index, null_value_id());
    } else {
      _attribute_vector->set(val_index, inverted_dictionary[values_copy[val_index]]);
    }
  }
}

template <typename T>
AllTypeVariant DictionarySegment<T>::operator[](const ChunkOffset chunk_offset) const {
  return _dictionary[_attribute_vector->get(chunk_offset)];
}

template <typename T>
T DictionarySegment<T>::get(const ChunkOffset chunk_offset) const {
  return _dictionary[_attribute_vector->get(chunk_offset)];
}

template <typename T>
std::optional<T> DictionarySegment<T>::get_typed_value(const ChunkOffset chunk_offset) const {
  if (_attribute_vector->get(chunk_offset) == null_value_id()) {
    return std::nullopt;
  }
  return _dictionary[_attribute_vector->get(chunk_offset)];
}

template <typename T>
const std::vector<T>& DictionarySegment<T>::dictionary() const {
  return _dictionary;
}

template <typename T>
std::shared_ptr<const AbstractAttributeVector> DictionarySegment<T>::attribute_vector() const {
  return _attribute_vector;
}

template <typename T>
ValueID DictionarySegment<T>::null_value_id() const {
  return static_cast<ValueID>(_dictionary.size());
}

template <typename T>
const T DictionarySegment<T>::value_of_value_id(const ValueID value_id) const {
  if (value_id == null_value_id()) {
    throw std::logic_error("Value is null");
  }

  return _dictionary[value_id];
}

template <typename T>
ValueID DictionarySegment<T>::lower_bound(const T value) const {
  auto lower_bound = std::lower_bound(_dictionary.begin(), _dictionary.end(), value);
  if (lower_bound == _dictionary.end()) {
    return INVALID_VALUE_ID;
  }
  return static_cast<ValueID>(lower_bound - _dictionary.begin());
}

template <typename T>
ValueID DictionarySegment<T>::lower_bound(const AllTypeVariant& value) const {
  // TODO(anyone): Actually i dont know if this is the desired behaviour...
  if (variant_is_null(value)) {
    return INVALID_VALUE_ID;
  }
  return lower_bound(type_cast<T>(value));
}

template <typename T>
ValueID DictionarySegment<T>::upper_bound(const T value) const {
  auto upper_bound = std::upper_bound(_dictionary.begin(), _dictionary.end(), value);
  if (upper_bound == _dictionary.end()) {
    return INVALID_VALUE_ID;
  }
  return static_cast<ValueID>(upper_bound - _dictionary.begin());
}

template <typename T>
ValueID DictionarySegment<T>::upper_bound(const AllTypeVariant& value) const {
  // TODO(anyone): Actually i dont know if this is the desired behaviour...
  if (variant_is_null(value)) {
    return INVALID_VALUE_ID;
  }
  return upper_bound(type_cast<T>(value));
}

template <typename T>
ChunkOffset DictionarySegment<T>::unique_values_count() const {
  return _dictionary.size();
}

template <typename T>
ChunkOffset DictionarySegment<T>::size() const {
  return static_cast<ChunkOffset>(_attribute_vector->size());
}

template <typename T>
size_t DictionarySegment<T>::estimate_memory_usage() const {
  return (_dictionary.size() * sizeof(T)) + (_attribute_vector->size() * _attribute_vector->width());
}

EXPLICITLY_INSTANTIATE_DATA_TYPES(DictionarySegment);

}  // namespace opossum
