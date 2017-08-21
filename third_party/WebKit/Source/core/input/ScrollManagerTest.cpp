// Copyright 2017 The Chromium Authors. All rights reserved.
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
  void SetHtmlInnerHTML(const char* html_content);
  void Scroll(double x, double y);

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

class ScrollEndBuilder : public WebGestureEvent {
 public:
  ScrollEndBuilder()
      : WebGestureEvent(WebInputEvent::kGestureScrollEnd,
                        WebInputEvent::kNoModifiers,
                        TimeTicks::Now().InSeconds()) {}
};

void ScrollManagerTest::SetUp() {
  dummy_page_holder_ = DummyPageHolder::Create(IntSize(300, 400));
}

void ScrollManagerTest::SetHtmlInnerHTML(const char* html_content) {
  GetDocument().documentElement()->setInnerHTML(String::FromUTF8(html_content));
  GetDocument().View()->UpdateAllLifecyclePhases();
}

void ScrollManagerTest::Scroll(double x, double y) {
  GetDocument().GetFrame()->GetEventHandler().HandleGestureScrollEvent(
      ScrollBeginBuilder(x, y));
  GetDocument().GetFrame()->GetEventHandler().HandleGestureScrollEvent(
      ScrollUpdateBuilder(x, y));
  GetDocument().GetFrame()->GetEventHandler().HandleGestureScrollEvent(
      ScrollEndBuilder());
}

class ScrollBoundaryBehaviorTest : public ScrollManagerTest {
 protected:
  void SetUp() override {
    ScrollManagerTest::SetUp();

    SetHtmlInnerHTML(
        "<div id='outer' style='height: 300px; width: 300px; overflow: "
        "scroll;'>"
        "  <div id='inner' style='height: 500px; width: 500px; overflow: "
        "scroll;'>"
        "    <div id='content' style='height: 700px; width: 700px;'>"
        "</div></div></div>");

    Element* outer = GetDocument().getElementById("outer");
    Element* inner = GetDocument().getElementById("inner");

    // Scrolls the outer element to its bottom-right extent, and makes sure the
    // inner element is at its top-left extent. So that if the gesture is
    // towards down-right, the inner element doesn't scroll, and we are able to
    // check if the scroll is propagated to the outer element.
    outer->setScrollLeft(200);
    outer->setScrollTop(200);
    ASSERT_EQ(outer->scrollLeft(), 200);
    ASSERT_EQ(outer->scrollTop(), 200);
    ASSERT_EQ(inner->scrollLeft(), 0);
    ASSERT_EQ(inner->scrollTop(), 0);
  }

  void SetInnerScrollBoundaryBehavior(EScrollBoundaryBehavior x,
                                      EScrollBoundaryBehavior y) {
    Element* inner = GetDocument().getElementById("inner");
    inner->MutableComputedStyle()->SetScrollBoundaryBehaviorX(x);
    inner->MutableComputedStyle()->SetScrollBoundaryBehaviorY(y);
  }
};

TEST_F(ScrollBoundaryBehaviorTest, AutoAllowsPropagation) {
  SetInnerScrollBoundaryBehavior(EScrollBoundaryBehavior::kAuto,
                                 EScrollBoundaryBehavior::kAuto);
  Scroll(100.0, 100.0);
  Element* outer = GetDocument().getElementById("outer");
  ASSERT_EQ(outer->scrollLeft(), 100);
  ASSERT_EQ(outer->scrollTop(), 100);
}

TEST_F(ScrollBoundaryBehaviorTest, ContainOnXPreventsPropagationsOnX) {
  SetInnerScrollBoundaryBehavior(EScrollBoundaryBehavior::kContain,
                                 EScrollBoundaryBehavior::kAuto);
  Scroll(100, 0.0);
  Element* outer = GetDocument().getElementById("outer");
  ASSERT_EQ(outer->scrollLeft(), 200);
  ASSERT_EQ(outer->scrollTop(), 200);
}

TEST_F(ScrollBoundaryBehaviorTest, ContainOnXAllowsPropagationsOnY) {
  SetInnerScrollBoundaryBehavior(EScrollBoundaryBehavior::kContain,
                                 EScrollBoundaryBehavior::kAuto);
  Scroll(0.0, 100.0);
  Element* outer = GetDocument().getElementById("outer");
  ASSERT_EQ(outer->scrollLeft(), 200);
  ASSERT_EQ(outer->scrollTop(), 100);
}

TEST_F(ScrollBoundaryBehaviorTest, ContainOnXPreventsDiagonalPropagations) {
  SetInnerScrollBoundaryBehavior(EScrollBoundaryBehavior::kContain,
                                 EScrollBoundaryBehavior::kAuto);
  Scroll(100.0, 100.0);
  Element* outer = GetDocument().getElementById("outer");
  ASSERT_EQ(outer->scrollLeft(), 200);
  ASSERT_EQ(outer->scrollTop(), 200);
}

TEST_F(ScrollBoundaryBehaviorTest, ContainOnYPreventsPropagationsOnY) {
  SetInnerScrollBoundaryBehavior(EScrollBoundaryBehavior::kAuto,
                                 EScrollBoundaryBehavior::kContain);
  Scroll(0.0, 100.0);
  Element* outer = GetDocument().getElementById("outer");
  ASSERT_EQ(outer->scrollLeft(), 200);
  ASSERT_EQ(outer->scrollTop(), 200);
}

TEST_F(ScrollBoundaryBehaviorTest, ContainOnYAllowsPropagationsOnX) {
  SetInnerScrollBoundaryBehavior(EScrollBoundaryBehavior::kAuto,
                                 EScrollBoundaryBehavior::kContain);
  Scroll(100.0, 0.0);
  Element* outer = GetDocument().getElementById("outer");
  ASSERT_EQ(outer->scrollLeft(), 100);
  ASSERT_EQ(outer->scrollTop(), 200);
}

TEST_F(ScrollBoundaryBehaviorTest, ContainOnYAPreventsDiagonalPropagations) {
  SetInnerScrollBoundaryBehavior(EScrollBoundaryBehavior::kAuto,
                                 EScrollBoundaryBehavior::kContain);
  Scroll(100.0, 100.0);
  Element* outer = GetDocument().getElementById("outer");
  ASSERT_EQ(outer->scrollLeft(), 200);
  ASSERT_EQ(outer->scrollTop(), 200);
}

};  // namespace blink
