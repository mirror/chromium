// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/timing/TextElementTiming.h"

#include <memory>

#include "core/dom/DOMHighResTimeStamp.h"
#include "core/testing/DummyPageHolder.h"
#include "core/timing/Performance.h"
#include "platform/testing/TestingPlatformSupport.h"
#include "platform/wtf/CurrentTime.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace blink {

class TextElementTimingTest : public ::testing::Test {
 protected:
  void SetUp() override {
    platform_->AdvanceClockSeconds(1);
    dummy_page_holder_ = DummyPageHolder::Create(IntSize(800, 600));
  }

  double AdvanceClockAndGetTime() {
    platform_->AdvanceClockSeconds(1);
    return MonotonicallyIncreasingTime();
  }

  Document& GetDocument() { return dummy_page_holder_->GetDocument(); }
  TextElementTiming& GetTextElementTiming() {
    return TextElementTiming::From(GetDocument());
  }

  void AppendDocumentElementsWithElementTiming(int new_elements,
                                               const char* elementtiming) {
    StringBuilder builder;
    for (int i = 0; i < new_elements; ++i) {
      builder.Append("<h1 elementtiming='");
      builder.Append(elementtiming);
      builder.Append("'>foo</h1>");
    }
    GetDocument().write(builder.ToString());
  }

  void AppendDocumentElementsWithoutElementTiming(int new_elements) {
    StringBuilder builder;
    for (int i = 0; i < new_elements; ++i)
      builder.Append("<h1>foo</h1>");
    GetDocument().write(builder.ToString());
  }

  void SimulateLayoutAndPaint() { GetDocument().UpdateStyleAndLayout(); }

  int NumTimingEntries() {
    return GetTextElementTiming().element_timing_entry_names_.size();
  }

  int NumPendingEntryIds() {
    return GetTextElementTiming().pending_entry_names_.size();
  }

  Performance* GetPerformanceInstance() {
    TextElementTiming& timing(TextElementTiming::From(GetDocument()));
    return timing.GetPerformanceInstance(timing.GetFrame());
  }

  ScopedTestingPlatformSupport<TestingPlatformSupportWithMockScheduler>
      platform_;

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

TEST_F(TextElementTimingTest, SubtreeSingleElementTiming) {
  GetDocument().write("<h1 elementtiming='foo'><b>bold text</b></h1>");
  SimulateLayoutAndPaint();
  EXPECT_EQ(NumPendingEntryIds(), 1);
  EXPECT_EQ(NumTimingEntries(), 0);
  GetTextElementTiming().DidCommitCompositorFrame();
  EXPECT_EQ(NumPendingEntryIds(), 0);
  EXPECT_EQ(NumTimingEntries(), 1);
}

TEST_F(TextElementTimingTest, DeeplyNestedSubtreeSingleElementTiming) {
  StringBuilder builder;
  builder.Append("<div elementtiming='foo'>");
  for (int i = 0; i < 20; ++i)
    builder.Append("<div>");
  builder.Append("div text");
  for (int i = 0; i < 20; ++i)
    builder.Append("</div>");
  builder.Append("</div>");

  GetDocument().write(builder.ToString());
  SimulateLayoutAndPaint();
  EXPECT_EQ(NumPendingEntryIds(), 1);
  EXPECT_EQ(NumTimingEntries(), 0);
  GetTextElementTiming().DidCommitCompositorFrame();
  EXPECT_EQ(NumPendingEntryIds(), 0);
  EXPECT_EQ(NumTimingEntries(), 1);
}

TEST_F(TextElementTimingTest, MultipleSubtreesWithElementTiming) {
  StringBuilder builder;
  builder.Append("<div elementtiming='foo1'>");
  builder.Append("  <div elementtiming='foo2'>text</div>");
  builder.Append("  <div elementtiming='foo3'>text</div>");
  builder.Append("</div>");
  GetDocument().write(builder.ToString());
  SimulateLayoutAndPaint();
  EXPECT_EQ(NumPendingEntryIds(), 3);
  EXPECT_EQ(NumTimingEntries(), 0);
  GetTextElementTiming().DidCommitCompositorFrame();
  EXPECT_EQ(NumPendingEntryIds(), 0);
  EXPECT_EQ(NumTimingEntries(), 3);
}

TEST_F(TextElementTimingTest, TestPerformanceElementTimingTimestamps) {
  EXPECT_EQ(MonotonicallyIncreasingTime(), 1.0);

  StringBuilder builder;
  builder.Append("<div elementtiming='foo1'>");
  builder.Append("  <div>text</div>");
  builder.Append("  <div elementtiming='foo2'>text</div>");
  builder.Append("  <div>text</div>");
  builder.Append("</div>");
  GetDocument().write(builder.ToString());
  SimulateLayoutAndPaint();

  EXPECT_EQ(NumPendingEntryIds(), 2);
  EXPECT_EQ(NumTimingEntries(), 0);
  double commit1_time = AdvanceClockAndGetTime();
  GetTextElementTiming().DidCommitCompositorFrame();
  EXPECT_EQ(NumPendingEntryIds(), 0);
  EXPECT_EQ(NumTimingEntries(), 2);

  Performance* performance = GetPerformanceInstance();
  PerformanceEntryVector performance_entries =
      performance->getEntriesByType("elementtiming");

  EXPECT_GT(commit1_time, 0.0);
  EXPECT_EQ(performance_entries.size(), size_t(2));
  EXPECT_EQ(performance_entries[0]->startTime(),
            performance->MonotonicTimeToDOMHighResTimeStamp(commit1_time));
  EXPECT_EQ(performance_entries[1]->startTime(),
            performance->MonotonicTimeToDOMHighResTimeStamp(commit1_time));

  // Add more elements and make sure the timestamps increase.
  builder.Append("<p elementtiming='foo3'><b>Dynamically added</b></p>");
  GetDocument().write(builder.ToString());
  double commit2_time = AdvanceClockAndGetTime();
  SimulateLayoutAndPaint();

  EXPECT_EQ(NumPendingEntryIds(), 1);
  EXPECT_EQ(NumTimingEntries(), 2);
  GetTextElementTiming().DidCommitCompositorFrame();
  EXPECT_EQ(NumPendingEntryIds(), 0);
  EXPECT_EQ(NumTimingEntries(), 3);

  performance_entries = performance->getEntriesByType("elementtiming");
  EXPECT_GT(commit2_time, commit1_time);
  // Entries are sorted in order of increasing startTime
  EXPECT_EQ(performance_entries.size(), size_t(3));
  EXPECT_EQ(performance_entries[0]->startTime(),
            performance->MonotonicTimeToDOMHighResTimeStamp(commit1_time));
  EXPECT_EQ(performance_entries[1]->startTime(),
            performance->MonotonicTimeToDOMHighResTimeStamp(commit1_time));
  EXPECT_EQ(performance_entries[2]->startTime(),
            performance->MonotonicTimeToDOMHighResTimeStamp(commit2_time));
}

}  // namespace blink
