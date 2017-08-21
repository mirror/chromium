// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/input/ScrollManager.h"

#include "core/dom/Document.h"
#include "core/dom/NodeComputedStyle.h"
#include "core/input/EventHandler.h"
#include "core/style/ComputedStyle.h"
#include "core/testing/DummyPageHolder.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace blink {

class ScrollManagerTest : public ::testing::Test {
 protected:
  void SetUp() override;

  Page& GetPage() const { return dummy_page_holder_->GetPage(); }
  Document& GetDocument() const { return dummy_page_holder_->GetDocument(); }
  FrameSelection& Selection() const {
    return GetDocument().GetFrame()->Selection();
  }
  void SetHtmlInnerHTML(const char* html_content);

 protected:
  std::unique_ptr<DummyPageHolder> dummy_page_holder_;
};

class ScrollBeginBuilder : public WebGestureEvent {
 public:
  ScrollBeginBuilder(double hint_x, double hint_y)
      : WebGestureEvent(WebInputEvent::kGestureScrollBegin,
                        WebInputEvent::kNoModifiers,
                        TimeTicks::Now().InSeconds()) {
    x = global_x = 20;
    y = global_y = 20;
    data.scroll_begin.delta_x_hint = hint_x;
    data.scroll_begin.delta_y_hint = hint_y;
    data.scroll_begin.pointer_count = 1;
    frame_scale_ = 1;
  }
};

class ScrollUpdateBuilder : public WebGestureEvent {
 public:
  ScrollUpdateBuilder(double delta_x, double delta_y)
      : WebGestureEvent(WebInputEvent::kGestureScrollUpdate,
                        WebInputEvent::kNoModifiers,
                        TimeTicks::Now().InSeconds()) {
    x = global_x = 20;
    y = global_y = 20;
    data.scroll_update.delta_x = delta_x;
    data.scroll_update.delta_y = delta_y;
    frame_scale_ = 1;
  }
};

void ScrollManagerTest::SetUp() {
  dummy_page_holder_ = DummyPageHolder::Create(IntSize(300, 400));
}

void ScrollManagerTest::SetHtmlInnerHTML(const char* html_content) {
  GetDocument().documentElement()->setInnerHTML(String::FromUTF8(html_content));
  GetDocument().View()->UpdateAllLifecyclePhases();
}

// Scrolls the outer element to its bottom-right extent, and makes sure the
// inner element is at its top-left extent. So that if the gesture is towards
// down-right, the inner element doesn't scroll, and we are able to check if
// the scroll is propagated to the outer element.
static void SetUpScrollBoundaryBehaviorPage(Element* outer, Element* inner) {
  outer->setScrollLeft(200);
  outer->setScrollTop(200);
  ASSERT_EQ(outer->scrollLeft(), 200);
  ASSERT_EQ(outer->scrollTop(), 200);
  ASSERT_EQ(inner->scrollLeft(), 0);
  ASSERT_EQ(inner->scrollTop(), 0);
}

TEST_F(ScrollManagerTest, ScrollBoundaryBehaviorPreventsPropagation) {
  SetHtmlInnerHTML(
      "<div id='outer' style='height: 300px; width: 300px; overflow: scroll;'>"
      "  <div id='inner' style='height: 500px; width: 500px; overflow: "
      "scroll;'>"
      "    <div id='content' style='height: 700px; width: 700px;'>"
      "</div></div></div>");

  Element* outer = GetDocument().getElementById("outer");
  Element* inner = GetDocument().getElementById("inner");

  // ScrollBoundaryBehaviorAuto shouldn't prevent scroll propagation.
  inner->MutableComputedStyle()->SetScrollBoundaryBehaviorX(
      EScrollBoundaryBehavior::kAuto);
  inner->MutableComputedStyle()->SetScrollBoundaryBehaviorY(
      EScrollBoundaryBehavior::kAuto);
  SetUpScrollBoundaryBehaviorPage(outer, inner);
  GetDocument().GetFrame()->GetEventHandler().HandleGestureScrollEvent(
      ScrollBeginBuilder(100.0, 100.0));
  GetDocument().GetFrame()->GetEventHandler().HandleGestureScrollEvent(
      ScrollUpdateBuilder(100.0, 100.0));
  ASSERT_EQ(outer->scrollLeft(), 100);
  ASSERT_EQ(outer->scrollTop(), 100);

  // ScrollBoundaryBehaviorContain on x should prevent propagations of scroll
  // on x.
  inner->MutableComputedStyle()->SetScrollBoundaryBehaviorX(
      EScrollBoundaryBehavior::kContain);
  inner->MutableComputedStyle()->SetScrollBoundaryBehaviorY(
      EScrollBoundaryBehavior::kAuto);
  SetUpScrollBoundaryBehaviorPage(outer, inner);
  GetDocument().GetFrame()->GetEventHandler().HandleGestureScrollEvent(
      ScrollBeginBuilder(100.0, 0.0));
  GetDocument().GetFrame()->GetEventHandler().HandleGestureScrollEvent(
      ScrollUpdateBuilder(100.0, 0.0));
  ASSERT_EQ(outer->scrollLeft(), 200);
  ASSERT_EQ(outer->scrollTop(), 200);

  // ScrollBoundaryBehaviorContain on x shouldn't prevent propagations of scroll
  // on y.
  SetUpScrollBoundaryBehaviorPage(outer, inner);
  GetDocument().GetFrame()->GetEventHandler().HandleGestureScrollEvent(
      ScrollBeginBuilder(0.0, 100.0));
  GetDocument().GetFrame()->GetEventHandler().HandleGestureScrollEvent(
      ScrollUpdateBuilder(0.0, 100.0));
  ASSERT_EQ(outer->scrollLeft(), 200);
  ASSERT_EQ(outer->scrollTop(), 100);

  // A scroll update with both x & y delta will adhere to the most restrictive
  // case.
  SetUpScrollBoundaryBehaviorPage(outer, inner);
  GetDocument().GetFrame()->GetEventHandler().HandleGestureScrollEvent(
      ScrollBeginBuilder(100.0, 100.0));
  GetDocument().GetFrame()->GetEventHandler().HandleGestureScrollEvent(
      ScrollUpdateBuilder(100.0, 100.0));
  ASSERT_EQ(outer->scrollLeft(), 200);
  ASSERT_EQ(outer->scrollTop(), 200);

  // ScrollBoundaryBehaviorContain on y shouldn't prevent propagations of scroll
  // on x.
  inner->MutableComputedStyle()->SetScrollBoundaryBehaviorX(
      EScrollBoundaryBehavior::kAuto);
  inner->MutableComputedStyle()->SetScrollBoundaryBehaviorY(
      EScrollBoundaryBehavior::kContain);
  SetUpScrollBoundaryBehaviorPage(outer, inner);
  GetDocument().GetFrame()->GetEventHandler().HandleGestureScrollEvent(
      ScrollBeginBuilder(100.0, 0.0));
  GetDocument().GetFrame()->GetEventHandler().HandleGestureScrollEvent(
      ScrollUpdateBuilder(100.0, 0.0));
  ASSERT_EQ(outer->scrollLeft(), 100);
  ASSERT_EQ(outer->scrollTop(), 200);

  // ScrollBoundaryBehaviorContain on y shouldn prevent propagations of scroll
  // on y.
  SetUpScrollBoundaryBehaviorPage(outer, inner);
  GetDocument().GetFrame()->GetEventHandler().HandleGestureScrollEvent(
      ScrollBeginBuilder(0.0, 100.0));
  GetDocument().GetFrame()->GetEventHandler().HandleGestureScrollEvent(
      ScrollUpdateBuilder(0.0, 100.0));
  ASSERT_EQ(outer->scrollLeft(), 200);
  ASSERT_EQ(outer->scrollTop(), 200);

  // A scroll update with both x & y delta will adhere to the most restrictive
  // case.
  SetUpScrollBoundaryBehaviorPage(outer, inner);
  GetDocument().GetFrame()->GetEventHandler().HandleGestureScrollEvent(
      ScrollBeginBuilder(100.0, 100.0));
  GetDocument().GetFrame()->GetEventHandler().HandleGestureScrollEvent(
      ScrollUpdateBuilder(100.0, 100.0));
  ASSERT_EQ(outer->scrollLeft(), 200);
  ASSERT_EQ(outer->scrollTop(), 200);
}

};  // namespace blink
