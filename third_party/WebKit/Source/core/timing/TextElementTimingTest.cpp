// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/timing/TextElementTiming.h"

#include <memory>

#include "core/testing/DummyPageHolder.h"
#include "platform/wtf/HashMap.h"
#include "platform/wtf/HashSet.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace blink {

class TextElementTimingTest : public ::testing::Test {
 protected:
  void SetUp() override {
    dummy_page_holder_ = DummyPageHolder::Create(IntSize(800, 600));
  }

  Document& GetDocument() { return dummy_page_holder_->GetDocument(); }
  TextElementTiming& GetTextElementTiming() {
    return TextElementTiming::From(GetDocument());
  }

  void AppendDocumentElementsWithElementTiming(int new_elements,
                                               const char* elementtiming) {
    StringBuilder builder;
    for (int i = 0; i < new_elements; i++) {
      builder.Append("<h1 elementtiming='");
      builder.Append(elementtiming);
      builder.Append("'>foo</h1>");
    }
    GetDocument().write(builder.ToString());
  }

  void AppendDocumentElementsWithoutElementTiming(int new_elements) {
    StringBuilder builder;
    for (int i = 0; i < new_elements; i++)
      builder.Append("<h1>foo</h1>");
    GetDocument().write(builder.ToString());
  }

  void SimulateLayoutAndPaint() { GetDocument().UpdateStyleAndLayout(); }

  int NumTimingEntries() {
    return GetTextElementTiming().timing_entries_.size();
  }

  int NumPendingEntryIds() {
    return GetTextElementTiming().pending_entry_ids_.size();
  }

 private:
  std::unique_ptr<DummyPageHolder> dummy_page_holder_;
};

TEST_F(TextElementTimingTest, TrackSingleTextElement) {
  AppendDocumentElementsWithElementTiming(1, "element1");
  SimulateLayoutAndPaint();
  EXPECT_EQ(NumPendingEntryIds(), 1);
  EXPECT_EQ(NumTimingEntries(), 0);
  GetTextElementTiming().DidCommitCompositorFrame();
  EXPECT_EQ(NumPendingEntryIds(), 0);
  EXPECT_EQ(NumTimingEntries(), 1);
}

TEST_F(TextElementTimingTest, TrackMultipleTextElements) {
  AppendDocumentElementsWithElementTiming(1, "element1");
  AppendDocumentElementsWithElementTiming(1, "element2");
  AppendDocumentElementsWithElementTiming(1, "element3");
  SimulateLayoutAndPaint();
  EXPECT_EQ(NumPendingEntryIds(), 3);
  EXPECT_EQ(NumTimingEntries(), 0);
  GetTextElementTiming().DidCommitCompositorFrame();
  EXPECT_EQ(NumPendingEntryIds(), 0);
  EXPECT_EQ(NumTimingEntries(), 3);
}

TEST_F(TextElementTimingTest, IgnoreDuplicateElementTimingIds) {
  AppendDocumentElementsWithElementTiming(10, "element1");
  SimulateLayoutAndPaint();
  EXPECT_EQ(NumPendingEntryIds(), 1);
  EXPECT_EQ(NumTimingEntries(), 0);
  GetTextElementTiming().DidCommitCompositorFrame();
  EXPECT_EQ(NumPendingEntryIds(), 0);
  EXPECT_EQ(NumTimingEntries(), 1);
}

TEST_F(TextElementTimingTest, IgnoreEmptyElementTimingIds) {
  AppendDocumentElementsWithElementTiming(1, "");
  SimulateLayoutAndPaint();
  EXPECT_EQ(NumPendingEntryIds(), 0);
  EXPECT_EQ(NumTimingEntries(), 0);
  GetTextElementTiming().DidCommitCompositorFrame();
  EXPECT_EQ(NumPendingEntryIds(), 0);
  EXPECT_EQ(NumTimingEntries(), 0);
}

TEST_F(TextElementTimingTest, IgnoreMissingElementTimingIds) {
  AppendDocumentElementsWithoutElementTiming(1);
  SimulateLayoutAndPaint();
  EXPECT_EQ(NumPendingEntryIds(), 0);
  EXPECT_EQ(NumTimingEntries(), 0);
  GetTextElementTiming().DidCommitCompositorFrame();
  EXPECT_EQ(NumPendingEntryIds(), 0);
  EXPECT_EQ(NumTimingEntries(), 0);
}

}  // namespace blink
