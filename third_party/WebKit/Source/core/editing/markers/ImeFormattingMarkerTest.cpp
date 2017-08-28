// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/editing/markers/ImeFormattingMarker.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace blink {

class ImeFormattingMarkerTest : public ::testing::Test {};

TEST_F(ImeFormattingMarkerTest, MarkerType) {
  DocumentMarker* marker = new ImeFormattingMarker(
      0, 1, Color::kTransparent, StyleableMarker::Thickness::kThin,
      Color::kTransparent);
  EXPECT_EQ(DocumentMarker::kImeFormatting, marker->GetType());
}

TEST_F(ImeFormattingMarkerTest, IsStyleableMarker) {
  DocumentMarker* marker = new ImeFormattingMarker(
      0, 1, Color::kTransparent, StyleableMarker::Thickness::kThin,
      Color::kTransparent);
  EXPECT_TRUE(IsStyleableMarker(*marker));
}

TEST_F(ImeFormattingMarkerTest, ConstructorAndGetters) {
  ImeFormattingMarker* marker = new ImeFormattingMarker(
      0, 1, Color::kDarkGray, StyleableMarker::Thickness::kThin, Color::kGray);
  EXPECT_EQ(Color::kDarkGray, marker->UnderlineColor());
  EXPECT_FALSE(marker->IsThick());
  EXPECT_EQ(Color::kGray, marker->BackgroundColor());

  ImeFormattingMarker* thick_marker = new ImeFormattingMarker(
      0, 1, Color::kDarkGray, StyleableMarker::Thickness::kThick, Color::kGray);
  EXPECT_EQ(true, thick_marker->IsThick());
}

}  // namespace blink
