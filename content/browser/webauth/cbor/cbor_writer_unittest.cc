// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/webauth/cbor/cbor_writer.h"

#include "base/memory/ptr_util.h"
#include "base/test/gtest_util.h"
#include "build/build_config.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

/* Leveraging RFC 7049 examples from
   https://github.com/cbor/test-vectors/blob/master/appendix_a.json. */
namespace content {

class CBORWriterTest : public testing::Test {
 protected:
  CBORWriter* cbor_writer_;
};

TEST_F(CBORWriterTest, TestWriteUint) {
  typedef struct {
    const uint64_t value;
    const char* cbor;
    const int num_bytes;
  } UintTestCase;

  static const UintTestCase kUintTestCases[] = {
      {0, "\x00", 1},
      {1, "\x01", 1},
      {10, "\x0a", 1},
      {23, "\x17", 1},
      {24, "\x18\x18", 2},
      {25, "\x18\x19", 2},
      {100, "\x18\x64", 2},
      {1000, "\x19\x03\xe8", 3},
      {1000000, "\x1a\x00\x0f\x42\x40", 5},
      {0xFFFFFFFF, "\x1a\xff\xff\xff\xff", 5},
  };

  for (const UintTestCase& test_case : kUintTestCases) {
    std::vector<uint8_t> cbor;
    CBORWriter::Write(CBORValue(test_case.value), &cbor);
    EXPECT_THAT(cbor,
                testing::ElementsAreArray(test_case.cbor, test_case.num_bytes));
  }
}

TEST_F(CBORWriterTest, TestWriteBytes) {
  typedef struct {
    const std::vector<uint8_t> bytes;
    const char* cbor;
    const int num_bytes;
  } BytesTestCase;

  static const BytesTestCase kBytesTestCases[] = {
      {{}, "\x40", 1}, {{0x01, 0x02, 0x03, 0x04}, "\x44\x01\x02\x03\x04", 5}};

  for (const BytesTestCase& test_case : kBytesTestCases) {
    std::vector<uint8_t> cbor;
    CBORWriter::Write(CBORValue(test_case.bytes), &cbor);
    EXPECT_THAT(cbor,
                testing::ElementsAreArray(test_case.cbor, test_case.num_bytes));
  }
}

TEST_F(CBORWriterTest, TestWriteString) {
  typedef struct {
    const std::string string;
    const char* cbor;
    const int num_bytes;
  } StringTestCase;

  static const StringTestCase kStringTestCases[] = {
      {"", "\x60", 1},
      {"a", "\x61\x61", 2},
      {"IETF", "\x64\x49\x45\x54\x46", 5},
      {"\"\\", "\x62\x22\x5c", 3},
      {"\xc3\xbc", "\x62\xc3\xbc", 3},
      {"\xe6\xb0\xb4", "\x63\xe6\xb0\xb4", 4},
      {"\xf0\x90\x85\x91", "\x64\xf0\x90\x85\x91", 5}};

  for (const StringTestCase& test_case : kStringTestCases) {
    std::vector<uint8_t> cbor;
    CBORWriter::Write(CBORValue(base::StringPiece(test_case.string)), &cbor);
    EXPECT_THAT(cbor,
                testing::ElementsAreArray(test_case.cbor, test_case.num_bytes));
  }
}

TEST_F(CBORWriterTest, TestWriteArray) {
  static const uint8_t kArrayTestCaseCbor[] = {
      0x98, 0x19, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
      0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12,
      0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x18, 0x18, 0x19};
  std::vector<uint8_t> cbor;
  std::vector<CBORValue> array;
  for (int i = 1; i <= 25; i++) {
    array.push_back(CBORValue(i));
  }
  CBORWriter::Write(CBORValue(array), &cbor);
  EXPECT_THAT(cbor, testing::ElementsAreArray(kArrayTestCaseCbor,
                                              arraysize(kArrayTestCaseCbor)));
}

TEST_F(CBORWriterTest, TestWriteMapWithMapBuilder) {
  static const uint8_t kMapTestCaseCbor[] = {
      0xa5, 0x61, 0x61, 0x61, 0x41, 0x61, 0x62, 0x61, 0x42, 0x61, 0x63,
      0x61, 0x43, 0x61, 0x64, 0x61, 0x44, 0x61, 0x65, 0x61, 0x45};
  std::vector<uint8_t> cbor;
  base::flat_map<std::string, std::unique_ptr<CBORValue>> map;
  map.emplace_hint(map.end(), "a", base::MakeUnique<CBORValue>(CBORValue("A")));
  map.emplace_hint(map.end(), "b", base::MakeUnique<CBORValue>(CBORValue("B")));
  map.emplace_hint(map.end(), "c", base::MakeUnique<CBORValue>(CBORValue("C")));
  map.emplace_hint(map.end(), "d", base::MakeUnique<CBORValue>(CBORValue("D")));
  map.emplace_hint(map.end(), "e", base::MakeUnique<CBORValue>(CBORValue("E")));
  CBORWriter::Write(CBORValue(map), &cbor);
  EXPECT_THAT(cbor, testing::ElementsAreArray(kMapTestCaseCbor,
                                              arraysize(kMapTestCaseCbor)));
}

TEST_F(CBORWriterTest, TestWriteMapWithMapValue) {
  static const uint8_t kMapTestCaseCbor[] = {
      0xa5, 0x61, 0x61, 0x61, 0x41, 0x61, 0x62, 0x61, 0x42, 0x61, 0x63,
      0x61, 0x43, 0x61, 0x64, 0x61, 0x44, 0x61, 0x65, 0x61, 0x45};
  std::vector<uint8_t> cbor;
  MapValue map_value;
  map_value.Insert("a", "A");
  map_value.Insert("b", "B");
  map_value.Insert("c", "C");
  map_value.Insert("d", "D");
  map_value.Insert("e", "E");
  CBORWriter::Write(map_value, &cbor);
  EXPECT_THAT(cbor, testing::ElementsAreArray(kMapTestCaseCbor,
                                              arraysize(kMapTestCaseCbor)));
}

TEST_F(CBORWriterTest, TestWriteMapWithArray) {
  static const uint8_t kMapArrayTestCaseCbor[] = {0xa2, 0x61, 0x61, 0x01, 0x61,
                                                  0x62, 0x82, 0x02, 0x03};
  std::vector<uint8_t> cbor;
  MapValue map_value;
  map_value.Insert("a", 1);
  std::vector<CBORValue> array;
  array.push_back(CBORValue(2));
  array.push_back(CBORValue(3));
  map_value.Insert("b", array);
  CBORWriter::Write(map_value, &cbor);
  EXPECT_THAT(cbor,
              testing::ElementsAreArray(kMapArrayTestCaseCbor,
                                        arraysize(kMapArrayTestCaseCbor)));
}

}  // namespace content
