// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/zucchini/image_index.h"

#include <stddef.h>

#include <numeric>
#include <vector>

#include "base/test/gtest_util.h"
#include "chrome/installer/zucchini/image_utils.h"
#include "chrome/installer/zucchini/test_reference_reader.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace zucchini {

class ImageIndexTest : public testing::Test {
 protected:
  ImageIndexTest()
      : buffer_(20),
        image_index_(ConstBufferView(buffer_.data(), buffer_.size()),
                     {ReferenceTypeTraits{2, TypeTag(0), PoolTag(0)},
                      ReferenceTypeTraits{4, TypeTag(1), PoolTag(0)},
                      ReferenceTypeTraits{3, TypeTag(2), PoolTag(1)}}) {
    std::iota(buffer_.begin(), buffer_.end(), 0);
  }

  void InsertReferences0() {
    EXPECT_TRUE(image_index_.InsertReferences(
        TypeTag(0), TestReferenceReader({{1, 0}, {8, 1}, {10, 2}})));
  }
  void InsertReferences1() {
    EXPECT_TRUE(image_index_.InsertReferences(TypeTag(1),
                                              TestReferenceReader({{3, 3}})));
  }
  void InsertReferences2() {
    EXPECT_TRUE(image_index_.InsertReferences(
        TypeTag(2), TestReferenceReader({{12, 4}, {17, 5}})));
  }
  void InsertAllReferences() {
    InsertReferences0();
    InsertReferences1();
    InsertReferences2();
  }

  std::vector<uint8_t> buffer_;
  ImageIndex image_index_;
};

TEST_F(ImageIndexTest, TypeAndPool) {
  InsertAllReferences();

  EXPECT_EQ(3U, image_index_.TypeCount());
  EXPECT_EQ(2U, image_index_.PoolCount());

  EXPECT_EQ(PoolTag(0), image_index_.GetPoolTag(TypeTag(0)));
  EXPECT_EQ(PoolTag(0), image_index_.GetPoolTag(TypeTag(1)));
  EXPECT_EQ(PoolTag(1), image_index_.GetPoolTag(TypeTag(2)));

  EXPECT_EQ(0U, image_index_.LabelBound(PoolTag(0)));
  EXPECT_EQ(0U, image_index_.LabelBound(PoolTag(1)));
}

TEST_F(ImageIndexTest, InvalidInsertReferences1) {
  // Overlap within the same reader.
  EXPECT_FALSE(image_index_.InsertReferences(
      TypeTag(0), TestReferenceReader({{1, 0}, {2, 0}})));
}

TEST_F(ImageIndexTest, InvalidInsertReferences2) {
  InsertReferences0();
  InsertReferences1();

  // Overlap across different readers.
  EXPECT_FALSE(image_index_.InsertReferences(TypeTag(2),
                                             TestReferenceReader({{11, 0}})));
}

TEST_F(ImageIndexTest, GetTypeAt) {
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
      2,  2,  2,     // ref 2
  };

  for (offset_t i = 0; i < image_index_.size(); ++i)
    EXPECT_EQ(TypeTag(expected[i]), image_index_.GetType(i));
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
      1, 0, 0,     // ref 2
  };

  for (offset_t i = 0; i < image_index_.size(); ++i)
    EXPECT_EQ(expected[i], image_index_.IsToken(i));
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
      1, 1, 1,     // ref 2
  };

  for (offset_t i = 0; i < image_index_.size(); ++i) {
    EXPECT_EQ(expected[i], image_index_.IsReference(i));
  }
}

}  // namespace zucchini
