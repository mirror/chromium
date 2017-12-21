// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/common/resource_request_body.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace content {

TEST(ResourceRequestBodyTest, TestBasicClone) {
  auto original = base::MakeRefCounted<ResourceRequestBody>();
  original->AppendBytes("hi", 2);
  original->AppendFileRange(base::FilePath(), 0ull, 10ull, base::Time());

  auto clone = original->Clone();

  ASSERT_EQ(clone->elements()->size(), 2u);
  storage::DataElement test;
  test.SetToBytes("hi", 2);
  EXPECT_EQ(clone->elements()->at(0), test);
  test.SetToFilePathRange(base::FilePath(), 0ull, 10ull, base::Time());
  EXPECT_EQ(clone->elements()->at(1), test);
}

// TODO(dmurph): Test that a data pipe getter clone works by consuming the data
// pipe from both the original and the cloned body.

}  // namespace content
