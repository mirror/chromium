// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <Foundation/NSObject.h>

#include "base/optional.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace base {

TEST(OptionalTestMac, StoreARCObject) {
  Optional<NSData*> a;
  EXPECT_FALSE(a.has_value());
  a = nullptr;
  EXPECT_TRUE(a.has_value());
}

}  // namespace base
