// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/cbor/cbor_writer.h"

#include "base/memory/ptr_util.h"
#include "base/test/gtest_util.h"
#include "base/values.h"
#include "build/build_config.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

/* Leveraging RFC 7049 examples from
   https://github.com/cbor/test-vectors/blob/master/appendix_a.json. */
namespace base {

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
    cbor_writer_ = new CBORWriter();
    cbor_writer_->WriteUint(test_case.value);
    EXPECT_THAT(cbor_writer_->Serialize(),
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
    cbor_writer_ = new CBORWriter();
    cbor_writer_->WriteBytes(test_case.bytes);
    EXPECT_THAT(cbor_writer_->Serialize(),
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
    cbor_writer_ = new CBORWriter();
    cbor_writer_->WriteString(test_case.string);
    EXPECT_THAT(cbor_writer_->Serialize(),
                testing::ElementsAreArray(test_case.cbor, test_case.num_bytes));
  }
}

TEST_F(CBORWriterTest, TestWriteMap) {
  static const uint8_t kMapTestCaseCbor[] = {
      0xa5, 0x61, 0x61, 0x61, 0x41, 0x61, 0x62, 0x61, 0x42, 0x61, 0x63,
      0x61, 0x43, 0x61, 0x64, 0x61, 0x44, 0x61, 0x65, 0x61, 0x45};

  cbor_writer_ = new CBORWriter();
  cbor_writer_->WriteMap(5);
  cbor_writer_->WriteString("a");
  cbor_writer_->WriteString("A");
  cbor_writer_->WriteString("b");
  cbor_writer_->WriteString("B");
  cbor_writer_->WriteString("c");
  cbor_writer_->WriteString("C");
  cbor_writer_->WriteString("d");
  cbor_writer_->WriteString("D");
  cbor_writer_->WriteString("e");
  cbor_writer_->WriteString("E");
  EXPECT_THAT(
      cbor_writer_->Serialize(),
      testing::ElementsAreArray(kMapTestCaseCbor, arraysize(kMapTestCaseCbor)));
}

TEST_F(CBORWriterTest, TestWriteMapNotEnoughPairs) {
  cbor_writer_ = new CBORWriter();
  cbor_writer_->WriteMap(5);
  cbor_writer_->WriteString("a");
  cbor_writer_->WriteString("A");
  cbor_writer_->WriteString("b");
  cbor_writer_->WriteString("B");
  EXPECT_DCHECK_DEATH(cbor_writer_->Serialize().data());
}

TEST_F(CBORWriterTest, TestWriteNestedMapNotEnoughPairs) {
  cbor_writer_ = new CBORWriter();
  cbor_writer_->WriteMap(5);
  cbor_writer_->WriteMap(1);
  cbor_writer_->WriteString("a");
  cbor_writer_->WriteString("A");
  EXPECT_DCHECK_DEATH(cbor_writer_->Serialize().data());
}

}  // namespace base
