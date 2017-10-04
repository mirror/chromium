// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/editing/SetSelectionOptions.h"

#include "core/editing/testing/EditingTestBase.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace blink {

class SetSelectionOptionsTest : public EditingTestBase {};

TEST_F(SetSelectionOptionsTest, BuilderTest) {
  SetSelectionOptions::Builder builder;

  builder.SetCursorAlignOnScroll(CursorAlignOnScroll::kAlways)
      .SetDoNotClearStrategy(true)
      .SetDoNotSetFocus(true)
      .SetGranularity(TextGranularity::kDocumentBoundary)
      .SetSetSelectionBy(SetSelectionBy::kUser)
      .SetShouldClearTypingStyle(true)
      .SetShouldCloseTyping(true)
      .SetShouldShowHandle(true)
      .SetShouldShrinkNextTap(true);

  SetSelectionOptions options = builder.Build();

  EXPECT_EQ(CursorAlignOnScroll::kAlways, options.GetCursorAlignOnScroll());
  EXPECT_EQ(true, options.DoNotClearStrategy());
  EXPECT_EQ(true, options.DoNotSetFocus());
  EXPECT_EQ(TextGranularity::kDocumentBoundary, options.Granularity());
  EXPECT_EQ(SetSelectionBy::kUser, options.GetSetSelectionBy());
  EXPECT_EQ(true, options.ShouldClearTypingStyle());
  EXPECT_EQ(true, options.ShouldCloseTyping());
  EXPECT_EQ(true, options.ShouldShowHandle());
  EXPECT_EQ(true, options.ShouldShrinkNextTap());
}

}  // namespace blink
