// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/elements/rect.h"

#include "base/memory/ptr_util.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/skia/include/core/SkColor.h"

namespace vr {

TEST(Rect, SetColorCorrectly) {
  auto rect = base::MakeUnique<Rect>();
  EXPECT_NE(SK_ColorCYAN, rect->edge_color());
  EXPECT_NE(SK_ColorCYAN, rect->center_color());

  rect->SetColor(SK_ColorCYAN);
  EXPECT_EQ(SK_ColorCYAN, rect->edge_color());
  EXPECT_EQ(SK_ColorCYAN, rect->center_color());

  rect->SetEdgeColor(SK_ColorRED);
  rect->SetCenterColor(SK_ColorBLUE);

  EXPECT_EQ(SK_ColorRED, rect->edge_color());
  EXPECT_EQ(SK_ColorBLUE, rect->center_color());
}

}  // namespace vr
