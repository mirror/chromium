// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/layers/region_map_utils.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace cc {
namespace {

TEST(RegionMapUtilsTest, UnionOfContainingRects) {
  Region r;
  Region r1;
  Region r2;
  RegionMap region_map;

  // A map with a containing rect gives back the containing rect.
  r1 = r = gfx::Rect(0, 0, 50, 50);
  r2 = gfx::Rect(20, 20, 10, 10);
  region_map[kTouchActionNone] = r1;
  region_map[kTouchActionPanLeft] = r2;
  EXPECT_EQ(r, region_map_utils::UnionOfRegions(region_map));
}

TEST(RegionMapUtilsTest, UnionOfNonContainingRects) {
  Region r;
  Region r1;
  Region r2;
  Region r3;
  RegionMap region_map;

  // A map with complex regions gives back the union of all mapped regions.
  r = r1 = gfx::Rect(0, 0, 50, 50);
  r2 = gfx::Rect(100, 0, 50, 50);
  r.Union(r2);
  r3 = gfx::Rect(0, 50, 50, 50);
  r.Union(r3);
  region_map[kTouchActionNone] = r1;
  region_map[kTouchActionPanLeft] = r2;
  region_map[kTouchActionPanRight] = r3;
  EXPECT_EQ(r, region_map_utils::UnionOfRegions(region_map));
}

}  // namespace
}  // namespace cc
