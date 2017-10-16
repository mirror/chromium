// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/wtf/WTF.h"
#include "platform/wtf/WTFThreadTestHelper.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace WTF {

TEST(AssertionsTest, IsMainThreadHelper) {
  EXPECT_TRUE(IsMainThread());
  {
    NotOnMainThread guard;
    EXPECT_FALSE(IsMainThread());
  }
  EXPECT_TRUE(IsMainThread());
};

}  // namespace WTF
