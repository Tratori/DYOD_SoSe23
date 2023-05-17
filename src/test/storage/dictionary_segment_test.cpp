#include "base_test.hpp"

#include "resolve_type.hpp"
#include "storage/abstract_attribute_vector.hpp"
#include "storage/abstract_segment.hpp"
#include "storage/dictionary_segment.hpp"

namespace opossum {

class StorageDictionarySegmentTest : public BaseTest {
 protected:
  std::shared_ptr<ValueSegment<int32_t>> value_segment_int{std::make_shared<ValueSegment<int32_t>>()};
  std::shared_ptr<ValueSegment<std::string>> value_segment_str{std::make_shared<ValueSegment<std::string>>(true)};
};

TEST_F(StorageDictionarySegmentTest, CompressSegmentString) {
  value_segment_str->append("Bill");
  value_segment_str->append("Steve");
  value_segment_str->append("Alexander");
  value_segment_str->append("Steve");
  value_segment_str->append("Hasso");
  value_segment_str->append("Bill");
  value_segment_str->append(NULL_VALUE);

  const auto dict_segment = std::make_shared<DictionarySegment<std::string>>(value_segment_str);

  // Test attribute_vector size.
  EXPECT_EQ(dict_segment->size(), 7);

  // Test dictionary size (uniqueness).
  EXPECT_EQ(dict_segment->unique_values_count(), 4);

  // Test sorting.
  const auto& dict = dict_segment->dictionary();
  EXPECT_EQ(dict[0], "Alexander");
  EXPECT_EQ(dict[1], "Bill");
  EXPECT_EQ(dict[2], "Hasso");
  EXPECT_EQ(dict[3], "Steve");

  // Test NULL value handling.
  EXPECT_EQ(dict_segment->attribute_vector()->get(6), dict_segment->null_value_id());
  EXPECT_EQ(dict_segment->get_typed_value(6), std::nullopt);
  EXPECT_THROW(dict_segment->get(6), std::logic_error);
  EXPECT_THROW(dict_segment->get(7), std::logic_error);
}

TEST_F(StorageDictionarySegmentTest, CompressSegmentDuplicateValues) {
  value_segment_int->append(1);
  value_segment_int->append(1);
  value_segment_int->append(2);
  value_segment_int->append(2);
  value_segment_int->append(1);
  value_segment_int->append(2);

  const auto dict_segment = std::make_shared<DictionarySegment<int32_t>>(value_segment_int);

  // Test attribute_vector size.
  EXPECT_EQ(dict_segment->size(), 6);

  // Test dictionary size (uniqueness).
  EXPECT_EQ(dict_segment->unique_values_count(), 2);

  // Test sorting.
  EXPECT_EQ(dict_segment->get(0), int32_t{1});
  EXPECT_EQ(dict_segment->get(1), 1);
  EXPECT_EQ(dict_segment->get(2), 2);
  EXPECT_EQ(dict_segment->get(3), 2);
  EXPECT_EQ(dict_segment->get(4), 1);
}

TEST_F(StorageDictionarySegmentTest, LowerUpperBound) {
  for (auto value = int16_t{0}; value <= 10; value += 2) {
    value_segment_int->append(value);
  }

  std::shared_ptr<AbstractSegment> segment;
  resolve_data_type("int", [&](auto type) {
    using Type = typename decltype(type)::type;
    segment = std::make_shared<DictionarySegment<Type>>(value_segment_int);
  });
  auto dict_segment = std::dynamic_pointer_cast<DictionarySegment<int32_t>>(segment);

  EXPECT_EQ(dict_segment->lower_bound(4), ValueID{2});
  EXPECT_EQ(dict_segment->upper_bound(4), ValueID{3});

  EXPECT_EQ(dict_segment->lower_bound(AllTypeVariant{4}), ValueID{2});
  EXPECT_EQ(dict_segment->upper_bound(AllTypeVariant{4}), ValueID{3});

  EXPECT_EQ(dict_segment->lower_bound(5), ValueID{3});
  EXPECT_EQ(dict_segment->upper_bound(5), ValueID{3});

  EXPECT_EQ(dict_segment->lower_bound(15), INVALID_VALUE_ID);
  EXPECT_EQ(dict_segment->upper_bound(15), INVALID_VALUE_ID);
}

std::shared_ptr<DictionarySegment<int32_t>> create_dictionary_segment(int32_t number_elements) {
  std::shared_ptr<ValueSegment<int32_t>> value_segment_int{std::make_shared<ValueSegment<int32_t>>()};
  for (auto value = int32_t{0}; value < number_elements; ++value) {
    value_segment_int->append(value);
  }
  std::shared_ptr<AbstractSegment> segment;
  resolve_data_type("int", [&](auto type) {
    using Type = typename decltype(type)::type;
    segment = std::make_shared<DictionarySegment<Type>>(value_segment_int);
  });
  auto dict_segment = std::dynamic_pointer_cast<DictionarySegment<int32_t>>(segment);
  return dict_segment;
}

TEST_F(StorageDictionarySegmentTest, CorrectWidth) {
  auto dict_segment = create_dictionary_segment(10);
  EXPECT_EQ(dict_segment->estimate_memory_usage(), 10 * sizeof(u_int32_t) + 10 * sizeof(u_int8_t));

  dict_segment = create_dictionary_segment(257);
  EXPECT_EQ(dict_segment->estimate_memory_usage(), 257 * sizeof(u_int32_t) + 257 * sizeof(u_int16_t));

  dict_segment = create_dictionary_segment(256 * 256);
  EXPECT_EQ(dict_segment->estimate_memory_usage(), 256 * 256 * sizeof(u_int32_t) + 256 * 256 * sizeof(u_int16_t));

  dict_segment = create_dictionary_segment(256 * 256 + 1);
  EXPECT_EQ(dict_segment->estimate_memory_usage(),
            (256 * 256 + 1) * sizeof(u_int32_t) + (256 * 256 + 1) * sizeof(u_int32_t));
}

}  // namespace opossum
