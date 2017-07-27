// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/zucchini/image_index.h"

#include <stddef.h>

#include <vector>

#include "chrome/installer/zucchini/image_utils.h"
#include "chrome/installer/zucchini/test_disassembler.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace zucchini {

class ImageIndexTest : public testing::Test {
 protected:
  ImageIndexTest()
      : buffer_(20),
        image_index_(ConstBufferView(buffer_.data(), buffer_.size())) {
    for (uint8_t i = 0; i < buffer_.size(); ++i) {
      buffer_[i] = i;
    }
  }

  void InsertReferences0() {
    EXPECT_TRUE(image_index_.InsertReferences(
        ReferenceTypeTraits{2, TypeTag(0), PoolTag(0)},
        TestReferenceReader({{1, 0}, {8, 0}, {10, 0}})));
  }
  void InsertReferences1() {
    EXPECT_TRUE(image_index_.InsertReferences(
        ReferenceTypeTraits{4, TypeTag(1), PoolTag(0)},
        TestReferenceReader({{3, 0}})));
  }
  void InsertReferences2() {
    EXPECT_TRUE(image_index_.InsertReferences(
        ReferenceTypeTraits{3, TypeTag(2), PoolTag(1)},
        TestReferenceReader({{12, 0}, {17, 0}})));
  }
  void InsertAllReferences() {
    InsertReferences0();
    InsertReferences1();
    InsertReferences2();
  }

  std::vector<uint8_t> buffer_;
  ImageIndex image_index_;
};

TEST_F(ImageIndexTest, TypePoolCount1) {
  EXPECT_EQ(0, image_index_.TypeCount());
  EXPECT_EQ(0, image_index_.PoolCount());
  InsertReferences0();
  EXPECT_EQ(1, image_index_.TypeCount());
  EXPECT_EQ(1, image_index_.PoolCount());
  InsertReferences1();
  EXPECT_EQ(2, image_index_.TypeCount());
  EXPECT_EQ(1, image_index_.PoolCount());
  InsertReferences2();
  EXPECT_EQ(3, image_index_.TypeCount());
  EXPECT_EQ(2, image_index_.PoolCount());
}

TEST_F(ImageIndexTest, TypePoolCount2) {
  EXPECT_EQ(0, image_index_.TypeCount());
  EXPECT_EQ(0, image_index_.PoolCount());
  InsertReferences2();
  EXPECT_EQ(3, image_index_.TypeCount());
  EXPECT_EQ(2, image_index_.PoolCount());
  InsertReferences0();
  EXPECT_EQ(3, image_index_.TypeCount());
  EXPECT_EQ(2, image_index_.PoolCount());
  InsertReferences1();
  EXPECT_EQ(3, image_index_.TypeCount());
  EXPECT_EQ(2, image_index_.PoolCount());
}

TEST_F(ImageIndexTest, InvalidInsertReferences1) {
  // Overlap within the same reader.
  EXPECT_FALSE(image_index_.InsertReferences(
      ReferenceTypeTraits{2, TypeTag(3), PoolTag(1)},
      TestReferenceReader({{1, 0}, {2, 0}})));
}

TEST_F(ImageIndexTest, InvalidInsertReferences2) {
  InsertAllReferences();

  // Overlap across different readers.
  EXPECT_FALSE(image_index_.InsertReferences(
      ReferenceTypeTraits{2, TypeTag(3), PoolTag(1)},
      TestReferenceReader({{17, 0}})));
}

TEST_F(ImageIndexTest, GetType) {
  InsertAllReferences();

  std::vector<int> expected = {
      -1,            // raw
      0,  0,         // ref 0
      1,  1,  1, 1,  // ref 1
      -1,            // raw
      0,  0,         // ref 0
      0,  0,         // ref 0
      2,  2,  2,     // ref 2
      -1, -1,        // raw
      2,  2,  2,     // ref 0
  };

  for (offset_t i = 0; i < image_index_.size(); ++i) {
    EXPECT_EQ(TypeTag(expected[i]), image_index_.GetType(i));
  }
}

TEST_F(ImageIndexTest, IsToken) {
  InsertAllReferences();

  std::vector<bool> expected = {
      1,           // raw
      1, 0,        // ref 0
      1, 0, 0, 0,  // ref 1
      1,           // raw
      1, 0,        // ref 0
      1, 0,        // ref 0
      1, 0, 0,     // ref 2
      1, 1,        // raw
      1, 0, 0,     // ref 0
  };

  for (offset_t i = 0; i < image_index_.size(); ++i) {
    EXPECT_EQ(expected[i], image_index_.IsToken(i));
  }
}

TEST_F(ImageIndexTest, IsReference) {
  InsertAllReferences();

  std::vector<bool> expected = {
      0,           // raw
      1, 1,        // ref 0
      1, 1, 1, 1,  // ref 1
      0,           // raw
      1, 1,        // ref 0
      1, 1,        // ref 0
      1, 1, 1,     // ref 2
      0, 0,        // raw
      1, 1, 1,     // ref 0
  };

  for (offset_t i = 0; i < image_index_.size(); ++i) {
    EXPECT_EQ(expected[i], image_index_.IsReference(i));
  }
}

TEST_F(ImageIndexTest, FindReference) {
  InsertAllReferences();

  EXPECT_EQ(base::nullopt, image_index_.FindReference(TypeTag(0), 0));
  EXPECT_EQ(Reference({1, 0}), *image_index_.FindReference(TypeTag(0), 1));
  EXPECT_EQ(Reference({1, 0}), *image_index_.FindReference(TypeTag(0), 2));
  EXPECT_EQ(base::nullopt, image_index_.FindReference(TypeTag(0), 3));
  EXPECT_EQ(base::nullopt, image_index_.FindReference(TypeTag(1), 1));
  EXPECT_EQ(base::nullopt, image_index_.FindReference(TypeTag(1), 2));
  EXPECT_EQ(Reference({10, 0}), *image_index_.FindReference(TypeTag(0), 10));
  EXPECT_EQ(Reference({10, 0}), *image_index_.FindReference(TypeTag(0), 11));
  EXPECT_EQ(base::nullopt, image_index_.FindReference(TypeTag(0), 12));
}

}  // namespace zucchini
