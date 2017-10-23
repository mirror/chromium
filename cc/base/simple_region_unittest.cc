// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/base/simple_region.h"

#include "cc/base/region.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace cc {

TEST(SimpleRegion, Basic) {
  SimpleRegion region;
  std::vector<gfx::Rect> rects = {gfx::Rect(0, 0, 100, 100),
                                  gfx::Rect(50, 50, 200, 200)};
  Region expected_region;
  for (const auto& rect : rects) {
    region.Union(rect);
    expected_region.Union(rect);
  }
  EXPECT_TRUE(region.Intersects(gfx::Rect(75, 75, 50, 50)));
  EXPECT_FALSE(region.Intersects(gfx::Rect(120, 0, 25, 25)));
  EXPECT_EQ(region.bounds(), gfx::Rect(0, 0, 250, 250));
  EXPECT_TRUE(region.EqualsForTesting(expected_region));
}

TEST(SimpleRegion, ExceedLimit) {
  SimpleRegion region;
  for (size_t i = 0; i < 300; ++i)
    region.Union(gfx::Rect(i, i, 1, 1));
  EXPECT_TRUE(region.Intersects(gfx::Rect(150, 150, 10, 10)));
  EXPECT_FALSE(region.Intersects(gfx::Rect(0, 100, 10, 10)));
  EXPECT_EQ(region.bounds(), gfx::Rect(0, 0, 300, 300));
}

}  // namespace cc
