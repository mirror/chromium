// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/paint/paint_image.h"

#include "cc/test/skia_common.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace cc {
namespace {

TEST(PaintImageTest, Subsetting) {
  PaintImage image = CreateDiscardablePaintImage(gfx::Size(100, 100));
  EXPECT_EQ(SkIRect::MakeWH(image.width(), image.height()),
            SkIRect::MakeWH(100, 100));

  PaintImage subset_1 = image.MakeSubset(SkIRect::MakeXYWH(25, 25, 50, 50));
  EXPECT_EQ(SkIRect::MakeWH(subset_1.width(), subset_1.height()),
            SkIRect::MakeWH(50, 50));
  EXPECT_EQ(subset_1.subset(), SkIRect::MakeXYWH(25, 25, 50, 50));

  PaintImage subset_2 = subset_1.MakeSubset(SkIRect::MakeXYWH(25, 25, 25, 25));
  EXPECT_EQ(SkIRect::MakeWH(subset_2.width(), subset_2.height()),
            SkIRect::MakeWH(25, 25));
  EXPECT_EQ(subset_2.subset(), SkIRect::MakeXYWH(50, 50, 25, 25));

  PaintImage subset_3 = subset_2.MakeSubset(SkIRect::MakeXYWH(10, 10, 25, 25));
  EXPECT_FALSE(subset_3);
}

}  // namespace
}  // namespace cc
