// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "content/browser/webauth/cbor/cbor_reader.h"

#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

/* Leveraging RFC 7049 examples from
   https://github.com/cbor/test-vectors/blob/master/appendix_a.json. */
namespace content {

TEST(CBORReaderTest, TestReadUint) {
  typedef struct {
    const uint64_t value;
    const std::vector<uint8_t> cbor_data;
  } UintTestCase;

  static const UintTestCase kUintTestCases[] = {
      {0, {0x00}},
      {1, {0x01}},
      {10, {0x0a}},
      {23, {0x17}},
      {24, {0x18, 0x18}},
      {25, {0x18, 0x19}},
      {100, {0x18, 0x64}},
      {1000, {0x19, 0x03, 0xe8}},
      {1000000, {0x1a, 0x00, 0x0f, 0x42, 0x40}},
      {0xFFFFFFFF, {0x1a, 0xff, 0xff, 0xff, 0xff}},
  };

  for (const UintTestCase& test_case : kUintTestCases) {
    base::Optional<CBORValue> cbor = CBORReader::Read(test_case.cbor_data);
    ASSERT_TRUE(cbor.has_value());
    ASSERT_EQ(cbor.value().type(), CBORValue::Type::UNSIGNED);
    EXPECT_EQ(cbor.value().GetUnsigned(), test_case.value);
  }
}

TEST(CBORReaderTest, TestReadBytes) {
  typedef struct {
    const std::vector<uint8_t> value;
    const std::vector<uint8_t> cbor_data;
  } ByteTestCase;

  static const ByteTestCase kByteStringTestCases[] = {
      {{}, {0x40}}, {{0x01, 0x02, 0x03, 0x04}, {0x44, 0x01, 0x02, 0x03, 0x04}},
  };

  for (const ByteTestCase& test_case : kByteStringTestCases) {
    base::Optional<CBORValue> cbor = CBORReader::Read(test_case.cbor_data);
    ASSERT_TRUE(cbor.has_value());
    ASSERT_EQ(cbor.value().type(), CBORValue::Type::BYTE_STRING);
    EXPECT_EQ(cbor.value().GetBytestring(), test_case.value);
  }
}

TEST(CBORReaderTest, TestReadString) {
  typedef struct {
    const std::string value;
    const std::vector<uint8_t> cbor_data;
  } StringTestCase;

  static const StringTestCase kStringTestCases[] = {
      {"", {0x60}},
      {"a", {0x61, 0x61}},
      {"IETF", {0x64, 0x49, 0x45, 0x54, 0x46}},
      {"\"\\", {0x62, 0x22, 0x5c}},
      {"\xc3\xbc", {0x62, 0xc3, 0xbc}},
      {"\xe6\xb0\xb4", {0x63, 0xe6, 0xb0, 0xb4}},
      {"\xf0\x90\x85\x91", {0x64, 0xf0, 0x90, 0x85, 0x91}},
  };

  for (const StringTestCase& test_case : kStringTestCases) {
    base::Optional<CBORValue> cbor = CBORReader::Read(test_case.cbor_data);
    ASSERT_TRUE(cbor.has_value());
    ASSERT_EQ(cbor.value().type(), CBORValue::Type::STRING);
    EXPECT_EQ(cbor.value().GetString(), test_case.value);
  }
}

TEST(CBORReaderTest, TestReadeArray) {
  static const std::vector<uint8_t> kArrayTestCaseCbor = {
      // clang-format off
      0x98, 0x19,  // array of 25 elements
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c,
        0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
        0x18, 0x18, 0x19,
      // clang-format on
  };

  base::Optional<CBORValue> cbor = CBORReader::Read(kArrayTestCaseCbor);
  ASSERT_TRUE(cbor.has_value());
  CBORValue cbor_array = std::move(cbor.value());
  ASSERT_EQ(cbor_array.type(), CBORValue::Type::ARRAY);
  ASSERT_THAT(cbor_array.GetArray(), testing::SizeIs(25));

  std::vector<CBORValue> array;
  for (int i = 0; i < 25; i++) {
    ASSERT_EQ(cbor_array.GetArray()[i].type(), CBORValue::Type::UNSIGNED);
    EXPECT_EQ(cbor_array.GetArray()[i].GetUnsigned(),
              static_cast<uint64_t>(i + 1));
  }
}

TEST(CBORReaderTest, TestReadMapWithMapValue) {
  static const std::vector<uint8_t> kMapTestCaseCbor = {
      // clang-format off
      0xa3,  // map of 3 pairs:
        0x60,  // ""
        0x61, 0x2e,  // "."

        0x61, 0x62,  // "b"
        0x61, 0x42,  // "B"

        0x62, 0x61, 0x61,  // "aa"
        0x62, 0x41, 0x41,  // "AA"
      // clang-format on
  };

  base::Optional<CBORValue> cbor = CBORReader::Read(kMapTestCaseCbor);
  ASSERT_TRUE(cbor.has_value());
  CBORValue cbor_val = std::move(cbor.value());
  ASSERT_EQ(cbor_val.type(), CBORValue::Type::MAP);
  ASSERT_EQ(cbor_val.GetMap().size(), 3u);

  ASSERT_EQ(cbor_val.GetMap().count(""), 1u);
  ASSERT_EQ(cbor_val.GetMap().find("")->second.type(), CBORValue::Type::STRING);
  EXPECT_EQ(cbor_val.GetMap().find("")->second.GetString(), ".");

  ASSERT_EQ(cbor_val.GetMap().count("b"), 1u);
  ASSERT_EQ(cbor_val.GetMap().find("b")->second.type(),
            CBORValue::Type::STRING);
  EXPECT_EQ(cbor_val.GetMap().find("b")->second.GetString(), "B");

  ASSERT_EQ(cbor_val.GetMap().count("aa"), 1u);
  ASSERT_EQ(cbor_val.GetMap().find("aa")->second.type(),
            CBORValue::Type::STRING);
  EXPECT_EQ(cbor_val.GetMap().find("aa")->second.GetString(), "AA");
}

TEST(CBORReaderTest, TestReadMapWithArray) {
  static const std::vector<uint8_t> kMapArrayTestCaseCbor = {
      // clang-format off
      0xa2,  // map of 2 pairs
        0x61, 0x61,  // "a"
        0x01,

        0x61, 0x62,  // "b"
        0x82,        // array with 2 elements
          0x02,
          0x03,
      // clang-format on
  };

  base::Optional<CBORValue> cbor = CBORReader::Read(kMapArrayTestCaseCbor);
  ASSERT_TRUE(cbor.has_value());
  CBORValue cbor_val = std::move(cbor.value());
  ASSERT_EQ(cbor_val.type(), CBORValue::Type::MAP);
  ASSERT_EQ(cbor_val.GetMap().size(), 2u);

  ASSERT_EQ(cbor_val.GetMap().count("a"), 1u);
  ASSERT_EQ(cbor_val.GetMap().find("a")->second.type(),
            CBORValue::Type::UNSIGNED);
  EXPECT_EQ(cbor_val.GetMap().find("a")->second.GetUnsigned(), 1u);

  ASSERT_EQ(cbor_val.GetMap().count("b"), 1u);
  ASSERT_EQ(cbor_val.GetMap().find("b")->second.type(), CBORValue::Type::ARRAY);

  CBORValue nested_array = cbor_val.GetMap().find("b")->second.Clone();
  ASSERT_EQ(nested_array.GetArray().size(), 2u);
  for (int i = 0; i < 2; i++) {
    ASSERT_THAT(nested_array.GetArray()[i].type(), CBORValue::Type::UNSIGNED);
    EXPECT_EQ(nested_array.GetArray()[i].GetUnsigned(),
              static_cast<uint64_t>(i + 2));
  }
}

TEST(CBORReaderTest, TestReadNestedMap) {
  static const std::vector<uint8_t> kNestedMapTestCase = {
      // clang-format off
      0xa2,  // map of 2 pairs
        0x61, 0x61,  // "a"
        0x01,

        0x61, 0x62,  // "b"
        0xa2,        // map of 2 pairs
          0x61, 0x63,  // "c"
          0x02,

          0x61, 0x64,  // "d"
          0x03,
      // clang-format on
  };

  base::Optional<CBORValue> cbor = CBORReader::Read(kNestedMapTestCase);
  ASSERT_TRUE(cbor.has_value());
  CBORValue cbor_val = std::move(cbor.value());
  ASSERT_EQ(cbor_val.type(), CBORValue::Type::MAP);
  ASSERT_EQ(cbor_val.GetMap().size(), 2u);
  ASSERT_EQ(cbor_val.GetMap().count("a"), 1u);
  ASSERT_EQ(cbor_val.GetMap().find("a")->second.type(),
            CBORValue::Type::UNSIGNED);
  EXPECT_EQ(cbor_val.GetMap().find("a")->second.GetUnsigned(), 1u);

  ASSERT_EQ(cbor_val.GetMap().count("b"), 1u);
  CBORValue nested_map = cbor_val.GetMap().find("b")->second.Clone();
  ASSERT_EQ(nested_map.type(), CBORValue::Type::MAP);
  ASSERT_EQ(nested_map.GetMap().size(), 2u);

  ASSERT_EQ(nested_map.GetMap().count("c"), 1u);
  ASSERT_EQ(nested_map.GetMap().find("c")->second.type(),
            CBORValue::Type::UNSIGNED);
  EXPECT_EQ(nested_map.GetMap().find("c")->second.GetUnsigned(), 2u);

  ASSERT_EQ(nested_map.GetMap().count("d"), 1u);
  ASSERT_EQ(nested_map.GetMap().find("d")->second.type(),
            CBORValue::Type::UNSIGNED);
  EXPECT_EQ(nested_map.GetMap().find("d")->second.GetUnsigned(), 3u);
}

TEST(CBORReaderTest, TestIncompleteCBORDataError) {
  static const std::vector<std::vector<uint8_t>> incomplete_cbor_list = {
      // Additional info byte corresponds to unsigned int that corresponds
      // to 2 additional bytes. But actual data encoded  in one byte.
      {0x19, 0x03},
      // CBOR bytestring of length 3 encoded with additional info of length 4.
      {0x44, 0x01, 0x02, 0x03},
      // CBOR string data "IETF" of length 4 encoded with additional info of
      // length 5.
      {0x65, 0x49, 0x45, 0x54, 0x46},
      // CBOR array of length 1 encoded with additional info of length 2.
      {0x82, 0x02},
      // CBOR map with single key value pair encoded with additional info of
      // length 2.
      {0xa2, 0x61, 0x61, 0x01},
  };

  for (auto incomplete_data : incomplete_cbor_list) {
    std::string error_msg;
    CBORReader::CBORDecoderError error_code;

    base::Optional<CBORValue> cbor =
        CBORReader::Read(incomplete_data, &error_code);
    EXPECT_FALSE(cbor.has_value());
    EXPECT_EQ(error_code, CBORReader::INCOMPLETE_CBOR_DATA_ERROR);
  }
}

TEST(CBORReaderTest, TestIncorrectKeyFormatError) {
  static const std::vector<uint8_t> kMapWithUintKey = {
      // clang-format off
      0xa2,        // map of 2 pairs
        0x01,      // key : 1
        0x02,      // value : 2

        0x61, 0x64,  // key : "d"
        0x03,        // value : 3
      // clang-format on
  };

  CBORReader::CBORDecoderError error_code;
  base::Optional<CBORValue> cbor =
      CBORReader::Read(kMapWithUintKey, &error_code);
  EXPECT_FALSE(cbor.has_value());
  EXPECT_EQ(error_code, CBORReader::INCORRECT_MAP_KEY_TYPE);
}

TEST(CBORReaderTest, TestUnknownAdditionalInfoError) {
  static const std::vector<std::vector<uint8_t>> kUnknownAdditionalInfoList = {
      {0x7C, 0x49, 0x45, 0x54, 0x46},
      {0x7D, 0x22, 0x5c},
      {0x7E, 0xc3, 0xbc},
      {0x7F, 0xe6, 0xb0, 0xb4}};

  for (auto incorrect_cbor : kUnknownAdditionalInfoList) {
    CBORReader::CBORDecoderError error_code;
    base::Optional<CBORValue> cbor =
        CBORReader::Read(incorrect_cbor, &error_code);
    EXPECT_FALSE(cbor.has_value());
    EXPECT_EQ(error_code, CBORReader::UNKNOWN_ADDITIONAL_INFO_ERROR);
  }
}

TEST(CBORReaderTest, TestTooMuchNestingError) {
  // Primitive CBOR data types (unsigned int, bytestring, string) and empty CBOR
  // array and map has zero nesting depth.
  static const std::vector<std::vector<uint8_t>> kZeroDepthCBORList = {
      // Unsigned int with value 100.
      {0x18, 0x64},
      // CBOR bytestring of length 4.
      {0x44, 0x01, 0x02, 0x03, 0x04},
      // CBOR string of corresponding to "IETF.
      {0x64, 0x49, 0x45, 0x54, 0x46},
      // Empty CBOR array.
      {0x80},
      // Empty CBOR Map
      {0xa0},
  };

  for (auto zero_depth_data : kZeroDepthCBORList) {
    CBORReader::CBORDecoderError error_code;
    base::Optional<CBORValue> cbor =
        CBORReader::Read(zero_depth_data, &error_code, true, 0);
    EXPECT_TRUE(cbor.has_value());
    EXPECT_EQ(error_code, CBORReader::CBOR_NO_ERROR);
  }

  // Corresponding to following CBOR structure with 2 nesting depth:
  //      {"a": 1,
  //       "b": [2, 3]}
  static const std::vector<uint8_t> kNestedCBORData = {
      // clang-format off
      0xa2,  // map of 2 pairs
        0x61, 0x61,  // "a"
        0x01,

        0x61, 0x62,  // "b"
        0x82,        // array with 2 elements
          0x02,
          0x03,
      // clang-format on
  };

  CBORReader::CBORDecoderError error_code;
  base::Optional<CBORValue> cbor_single_layer_max =
      CBORReader::Read(kNestedCBORData, &error_code, true, 1);
  EXPECT_FALSE(cbor_single_layer_max.has_value());
  EXPECT_EQ(error_code, CBORReader::TOO_MUCH_NESTING_ERROR);

  base::Optional<CBORValue> cbor_double_layer_max =
      CBORReader::Read(kNestedCBORData, &error_code, true, 2);
  EXPECT_TRUE(cbor_double_layer_max.has_value());
  EXPECT_EQ(error_code, CBORReader::CBOR_NO_ERROR);
}

TEST(CBORReaderTest, TestOutOfOrderKeyError) {
  static const std::vector<uint8_t> kMapWithUnsortedKey = {
      // clang-format off
      0xa6,  // map of 6 pairs:
        0x60,  // ""
        0x61, 0x2e,  // "."

        0x61, 0x62,  // "b"
        0x61, 0x42,  // "B"

        0x61, 0x63,  // "c"
        0x61, 0x43,  // "C"

        0x61, 0x64,  // "d"
        0x61, 0x44,  // "D"

        0x61, 0x61,  // "a" (Out of order key)
        0x61, 0x45,  // "E"

        0x62, 0x61, 0x61,  // "aa"
        0x62, 0x41, 0x41,  // "AA"
      // clang-format on
  };

  std::string error_msg;
  CBORReader::CBORDecoderError error_code;

  base::Optional<CBORValue> cbor =
      CBORReader::Read(kMapWithUnsortedKey, &error_code, true);
  EXPECT_FALSE(cbor.has_value());
  EXPECT_EQ(error_code, CBORReader::OUT_OF_ORDER_KEY_ERROR);
}

TEST(CBORReaderTest, TestDuplicateKeyError) {
  static const std::vector<uint8_t> kMapWithDuplicateKey = {
      // clang-format off
      0xa6,  // map of 6 pairs:
        0x60,  // ""
        0x61, 0x2e,  // "."

        0x61, 0x62,  // "b"
        0x61, 0x42,  // "B"

        0x61, 0x62,  // "b" (Duplicate key)
        0x61, 0x43,  // "C"

        0x61, 0x64,  // "d"
        0x61, 0x44,  // "D"

        0x61, 0x65,  // "e"
        0x61, 0x44,  // "D"

        0x62, 0x61, 0x61,  // "aa"
        0x62, 0x41, 0x41,  // "AA"
      // clang-format on
  };

  std::string error_msg;
  CBORReader::CBORDecoderError error_code;

  base::Optional<CBORValue> cbor =
      CBORReader::Read(kMapWithDuplicateKey, &error_code, true);
  EXPECT_FALSE(cbor.has_value());
  EXPECT_EQ(error_code, CBORReader::DUPLICATE_KEY_ERROR);
}

// Leveraging Markus Kuhn’s UTF-8 decoder stress test. See
// http://www.cl.cam.ac.uk/~mgk25/ucs/examples/UTF-8-test.txt for details.
TEST(CBORReaderTest, TestIncorrectStringEncodingError) {
  // Last 4 bytes correspond to "" (section 2.3.4 of the stress test).
  std::vector<uint8_t> example_utf8_byte{0x64, 0x00, 0x00, 0x00, 0x7F};
  std::string error_msg;
  CBORReader::CBORDecoderError error_code;
  base::Optional<CBORValue> correctly_encoded_cbor =
      CBORReader::Read(example_utf8_byte, &error_code);
  ASSERT_TRUE(correctly_encoded_cbor.has_value());
  ASSERT_EQ(error_code, CBORReader::CBOR_NO_ERROR);

  // Incorrect UTF8 encoding referenced by section 3.5.3 of the stress test.
  std::vector<uint8_t> impossible_utf_byte{0x64, 0xfe, 0xfe, 0xff, 0xff};
  base::Optional<CBORValue> incorrectly_encoded_cbor =
      CBORReader::Read(impossible_utf_byte, &error_code);
  ASSERT_FALSE(incorrectly_encoded_cbor.has_value());
  ASSERT_EQ(error_code, CBORReader::INVALID_UTF8_ERROR);
}

TEST(CBORReaderTest, TestExtraneousCBORDataError) {
  static const std::vector<std::vector<uint8_t>> zero_padded_cbor_list = {
      // 3 byte long unsigned int encoded with additional info of 2 byte length.
      {0x19, 0x03, 0x05, 0x00},
      // CBOR bytestring of length 5 encoded with additional info of length 4.
      {0x44, 0x01, 0x02, 0x03, 0x04, 0x00},
      // CBOR string of length 5 encoded with additional info of length 4.
      {0x64, 0x49, 0x45, 0x54, 0x46, 0x00},
      // CBOR array of length 3 encoded with additional info of length 2.
      {0x82, 0x01, 0x02, 0x00},
      // CBOR map with 2 pairs encoded with additional info of length 1.
      {0xa1, 0x61, 0x63, 0x02, 0x61, 0x64, 0x03},
  };

  for (auto extraneous_cbor_data : zero_padded_cbor_list) {
    std::string error_msg;
    CBORReader::CBORDecoderError error_code;

    base::Optional<CBORValue> cbor =
        CBORReader::Read(extraneous_cbor_data, &error_code, true);
    EXPECT_FALSE(cbor.has_value());
    EXPECT_EQ(error_code, CBORReader::EXTRANEOUS_DATA_ERROR);
  }
}

}  // namespace content
