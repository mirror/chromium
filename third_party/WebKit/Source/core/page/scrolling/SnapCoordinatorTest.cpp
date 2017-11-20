// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/page/scrolling/SnapCoordinator.h"

#include <gtest/gtest.h>
#include <memory>
#include "core/dom/Document.h"
#include "core/dom/Element.h"
#include "core/frame/LocalFrameView.h"
#include "core/html/HTMLElement.h"
#include "core/layout/LayoutBox.h"
#include "core/style/ComputedStyle.h"
#include "core/testing/DummyPageHolder.h"
#include "platform/scroll/ScrollTypes.h"
#include "platform/testing/RuntimeEnabledFeaturesTestHelpers.h"

namespace blink {

using HTMLNames::styleAttr;

typedef bool TestParamRootLayerScrolling;
class SnapCoordinatorTest
    : public ::testing::TestWithParam<TestParamRootLayerScrolling>,
      private ScopedRootLayerScrollingForTest {
 protected:
  SnapCoordinatorTest() : ScopedRootLayerScrollingForTest(GetParam()) {}

  void SetUp() override {
    page_holder_ = DummyPageHolder::Create();

    SetHTML(R"HTML(
      <style>
          #snap-container {
              height: 1000px;
              width: 1000px;
              overflow: scroll;
              scroll-snap-type: both mandatory;
          }
          #snap-element-fixed-position {
               position: fixed;
          }
      </style>
      <body>
        <div id='snap-container'>
          <div id='snap-element'></div>
          <div id='intermediate'>
             <div id='nested-snap-element'></div>
          </div>
          <div id='snap-element-fixed-position'></div>
          <div style='width:2000px; height:2000px;'></div>
        </div>
      </body>
    )HTML");
    GetDocument().UpdateStyleAndLayout();
  }

  void TearDown() override { page_holder_ = nullptr; }

  Document& GetDocument() { return page_holder_->GetDocument(); }

  void SetHTML(const char* html_content) {
    GetDocument().documentElement()->SetInnerHTMLFromString(html_content);
  }

  Element& SnapContainer() {
    return *GetDocument().getElementById("snap-container");
  }

  unsigned SizeOfSnapAreas(const ContainerNode& node) {
    if (node.GetLayoutBox()->SnapAreas())
      return node.GetLayoutBox()->SnapAreas()->size();
    return 0U;
  }

  void SetUpSingleSnapArea() {
    SetHTML(R"HTML(
      <style>
      #scroller {
        width: 140px;
        height: 160px;
        padding: 0px;
        scroll-snap-type: both mandatory;
        scroll-padding: 10px;
        overflow: scroll;
      }
      #container {
        margin: 0px;
        padding: 0px;
        width: 500px;
        height: 500px;
      }
      #area {
        position: relative;
        top: 200px;
        left: 200px;
        width: 100px;
        height: 100px;
        scroll-snap-margin: 8px;
      }
      </style>
      <div id="scroller">
        <div id="container">
          <div id="area"></div>
        </div>
      </div>
      )HTML");
    GetDocument().UpdateStyleAndLayout();
  }

  std::unique_ptr<DummyPageHolder> page_holder_;
};

INSTANTIATE_TEST_CASE_P(All, SnapCoordinatorTest, ::testing::Bool());

TEST_P(SnapCoordinatorTest, SimpleSnapElement) {
  Element& snap_element = *GetDocument().getElementById("snap-element");
  snap_element.setAttribute(styleAttr, "scroll-snap-align: start;");
  GetDocument().UpdateStyleAndLayout();

  EXPECT_EQ(1U, SizeOfSnapAreas(SnapContainer()));
}

TEST_P(SnapCoordinatorTest, NestedSnapElement) {
  Element& snap_element = *GetDocument().getElementById("nested-snap-element");
  snap_element.setAttribute(styleAttr, "scroll-snap-align: start;");
  GetDocument().UpdateStyleAndLayout();

  EXPECT_EQ(1U, SizeOfSnapAreas(SnapContainer()));
}

TEST_P(SnapCoordinatorTest, NestedSnapElementCaptured) {
  Element& snap_element = *GetDocument().getElementById("nested-snap-element");
  snap_element.setAttribute(styleAttr, "scroll-snap-align: start;");

  Element* intermediate = GetDocument().getElementById("intermediate");
  intermediate->setAttribute(styleAttr, "overflow: scroll;");

  GetDocument().UpdateStyleAndLayout();

  // Intermediate scroller captures nested snap elements first so ancestor
  // does not get them.
  EXPECT_EQ(0U, SizeOfSnapAreas(SnapContainer()));
  EXPECT_EQ(1U, SizeOfSnapAreas(*intermediate));
}

TEST_P(SnapCoordinatorTest, PositionFixedSnapElement) {
  Element& snap_element =
      *GetDocument().getElementById("snap-element-fixed-position");
  snap_element.setAttribute(styleAttr, "scroll-snap-align: start;");
  GetDocument().UpdateStyleAndLayout();

  // Position fixed elements are contained in document and not its immediate
  // ancestor scroller. They cannot be a valid snap destination so they should
  // not contribute snap points to their immediate snap container or document
  // See: https://lists.w3.org/Archives/Public/www-style/2015Jun/0376.html
  EXPECT_EQ(0U, SizeOfSnapAreas(SnapContainer()));

  Element* body = GetDocument().ViewportDefiningElement();
  EXPECT_EQ(0U, SizeOfSnapAreas(*body));
}

TEST_P(SnapCoordinatorTest, UpdateStyleForSnapElement) {
  Element& snap_element = *GetDocument().getElementById("snap-element");
  snap_element.setAttribute(styleAttr, "scroll-snap-align: start;");
  GetDocument().UpdateStyleAndLayout();

  EXPECT_EQ(1U, SizeOfSnapAreas(SnapContainer()));

  snap_element.remove();
  GetDocument().UpdateStyleAndLayout();

  EXPECT_EQ(0U, SizeOfSnapAreas(SnapContainer()));

  // Add a new snap element
  Element& container = *GetDocument().getElementById("snap-container");
  container.SetInnerHTMLFromString(R"HTML(
    <div style='scroll-snap-align: start;'>
        <div style='width:2000px; height:2000px;'></div>
    </div>
  )HTML");
  GetDocument().UpdateStyleAndLayout();

  EXPECT_EQ(1U, SizeOfSnapAreas(SnapContainer()));
}

TEST_P(SnapCoordinatorTest, LayoutViewCapturesWhenBodyElementViewportDefining) {
  SetHTML(R"HTML(
    <style>
    body {
        overflow: scroll;
        scroll-snap-type: both mandatory;
        height: 1000px;
        width: 1000px;
        margin: 5px;
    }
    </style>
    <body>
        <div id='snap-element' style='scroll-snap-align: start;></div>
        <div id='intermediate'>
            <div id='nested-snap-element'
                style='scroll-snap-align: start;'></div>
        </div>
        <div style='width:2000px; height:2000px;'></div>
    </body>
  )HTML");

  GetDocument().UpdateStyleAndLayout();

  // Sanity check that body is the viewport defining element
  EXPECT_EQ(GetDocument().body(), GetDocument().ViewportDefiningElement());

  // When body is viewport defining and overflows then any snap points on the
  // body element will be captured by layout view as the snap container.
  EXPECT_EQ(2U, SizeOfSnapAreas(GetDocument()));
  EXPECT_EQ(0U, SizeOfSnapAreas(*(GetDocument().body())));
  EXPECT_EQ(0U, SizeOfSnapAreas(*(GetDocument().documentElement())));
}

TEST_P(SnapCoordinatorTest,
       LayoutViewCapturesWhenDocumentElementViewportDefining) {
  SetHTML(R"HTML(
    <style>
    :root {
        overflow: scroll;
        scroll-snap-type: both mandatory;
        height: 500px;
        width: 500px;
    }
    body {
        margin: 5px;
    }
    </style>
    <html>
       <body>
           <div id='snap-element' style='scroll-snap-align: start;></div>
           <div id='intermediate'>
             <div id='nested-snap-element'
                 style='scroll-snap-align: start;'></div>
          </div>
          <div style='width:2000px; height:2000px;'></div>
       </body>
    </html>
  )HTML");

  GetDocument().UpdateStyleAndLayout();

  // Sanity check that document element is the viewport defining element
  EXPECT_EQ(GetDocument().documentElement(),
            GetDocument().ViewportDefiningElement());

  // When body is viewport defining and overflows then any snap points on the
  // the document element will be captured by layout view as the snap
  // container.
  EXPECT_EQ(2U, SizeOfSnapAreas(GetDocument()));
  EXPECT_EQ(0U, SizeOfSnapAreas(*(GetDocument().body())));
  EXPECT_EQ(0U, SizeOfSnapAreas(*(GetDocument().documentElement())));
}

TEST_P(SnapCoordinatorTest,
       BodyCapturesWhenBodyOverflowAndDocumentElementViewportDefining) {
  SetHTML(R"HTML(
    <style>
    :root {
        overflow: scroll;
        scroll-snap-type: both mandatory;
        height: 500px;
        width: 500px;
    }
    body {
        overflow: scroll;
        scroll-snap-type: both mandatory;
        height: 1000px;
        width: 1000px;
        margin: 5px;
    }
    </style>
    <html>
       <body style='overflow: scroll; scroll-snap-type: both mandatory;
    height:1000px; width:1000px;'>
           <div id='snap-element' style='scroll-snap-align: start;></div>
           <div id='intermediate'>
             <div id='nested-snap-element'
                 style='scroll-snap-align: start;'></div>
          </div>
          <div style='width:2000px; height:2000px;'></div>
       </body>
    </html>
  )HTML");

  GetDocument().UpdateStyleAndLayout();

  // Sanity check that document element is the viewport defining element
  EXPECT_EQ(GetDocument().documentElement(),
            GetDocument().ViewportDefiningElement());

  // When body and document elements are both scrollable then body element
  // should capture snap points defined on it as opposed to layout view.
  Element& body = *GetDocument().body();
  EXPECT_EQ(2U, SizeOfSnapAreas(body));
}

#define CHECK_CONTAINER(c1, c2)                                    \
  {                                                                \
    ASSERT_EQ(c1.max_offset.x(), c2.max_offset.x());               \
    ASSERT_EQ(c1.max_offset.y(), c2.max_offset.y());               \
    ASSERT_EQ(c1.scroll_snap_type, c2.scroll_snap_type);           \
    ASSERT_EQ(c1.snap_area_list.size(), c2.snap_area_list.size()); \
  }

#define CHECK_AREA(a1, a2)                             \
  {                                                    \
    ASSERT_EQ(a1.snap_axis, a2.snap_axis);             \
    ASSERT_EQ(a1.snap_offset.x(), a2.snap_offset.x()); \
    ASSERT_EQ(a1.snap_offset.y(), a2.snap_offset.y()); \
    ASSERT_EQ(a1.must_snap, a1.must_snap);             \
  }

TEST_P(SnapCoordinatorTest, StartAlignmentCalculation) {
  SetUpSingleSnapArea();
  Element* area_element = GetDocument().getElementById("area");
  area_element->setAttribute(styleAttr, "scroll-snap-align: start;");
  GetDocument().UpdateStyleAndLayout();
  Element* scroller_element = GetDocument().getElementById("scroller");
  SnapCoordinator* snap_coordinator = GetDocument().GetSnapCoordinator();
  SnapContainerData actual_container =
      snap_coordinator->CalculateSnapContainerData(
          scroller_element->GetLayoutBox());

  // #container.width - #scroller.width
  double scrollable_x = 500 - 140;

  // #container.height - #scroller.height
  double scrollable_y = 500 - 160;

  // (#area.left - #area.scroll-snap-margin) - (#scroller.scroll-padding)
  double snap_offset_x = (200 - 8) - 10;

  // (#area.top - #area.scroll-snap-margin) - (#scroller.scroll-padding)
  double snap_offset_y = (200 - 8) - 10;

  bool must_snap = false;

  SnapContainerData expected_container(
      ScrollSnapType(false, SnapAxis::kBoth, SnapStrictness::kMandatory),
      gfx::ScrollOffset(scrollable_x, scrollable_y));
  SnapAreaData expected_area(SnapAxis::kBoth,
                             gfx::ScrollOffset(snap_offset_x, snap_offset_y),
                             must_snap);
  expected_container.AddSnapAreaData(expected_area);

  CHECK_CONTAINER(actual_container, expected_container);
  CHECK_AREA(actual_container.snap_area_list[0], expected_area);
}

TEST_P(SnapCoordinatorTest, CenterAlignmentCalculation) {
  SetUpSingleSnapArea();
  Element* area_element = GetDocument().getElementById("area");
  area_element->setAttribute(styleAttr, "scroll-snap-align: center;");
  GetDocument().UpdateStyleAndLayout();
  Element* scroller_element = GetDocument().getElementById("scroller");
  SnapCoordinator* snap_coordinator = GetDocument().GetSnapCoordinator();
  SnapContainerData actual_container =
      snap_coordinator->CalculateSnapContainerData(
          scroller_element->GetLayoutBox());

  // #container.width - #scroller.width
  double scrollable_x = 500 - 140;

  // #container.height - #scroller.height
  double scrollable_y = 500 - 160;

  // (#area.left + #area.right) / 2 - (#scroller.left + #scroller.right) / 2
  double snap_offset_x = (200 + (200 + 100)) / 2 - 140 / 2;

  // (#area.top + #area.bottom) / 2 - (#scroller.top + #scroller.bottom) / 2
  double snap_offset_y = (200 + (200 + 100)) / 2 - 160 / 2;

  bool must_snap = false;

  SnapContainerData expected_container(
      ScrollSnapType(false, SnapAxis::kBoth, SnapStrictness::kMandatory),
      gfx::ScrollOffset(scrollable_x, scrollable_y));
  SnapAreaData expected_area(SnapAxis::kBoth,
                             gfx::ScrollOffset(snap_offset_x, snap_offset_y),
                             must_snap);
  expected_container.AddSnapAreaData(expected_area);

  CHECK_CONTAINER(actual_container, expected_container);
  CHECK_AREA(actual_container.snap_area_list[0], expected_area);
}

TEST_P(SnapCoordinatorTest, EndAlignmentCalculation) {
  SetUpSingleSnapArea();
  Element* area_element = GetDocument().getElementById("area");
  area_element->setAttribute(styleAttr, "scroll-snap-align: end;");
  GetDocument().UpdateStyleAndLayout();
  Element* scroller_element = GetDocument().getElementById("scroller");
  SnapCoordinator* snap_coordinator = GetDocument().GetSnapCoordinator();
  SnapContainerData actual_container =
      snap_coordinator->CalculateSnapContainerData(
          scroller_element->GetLayoutBox());

  // #container.width - #scroller.width
  double scrollable_x = 500 - 140;

  // #container.height - #scroller.height
  double scrollable_y = 500 - 160;

  // (#area.right + #area.scroll-snap-margin)
  // - (#scroller.right - #scroller.scroll-padding)
  double snap_offset_x = (200 + 100 + 8) - (140 - 10);

  // (#area.bottom + #area.scroll-snap-margin)
  // - (#scroller.bottom - #scroller.scroll-padding)
  double snap_offset_y = (200 + 100 + 8) - (160 - 10);

  bool must_snap = false;

  SnapContainerData expected_container(
      ScrollSnapType(false, SnapAxis::kBoth, SnapStrictness::kMandatory),
      gfx::ScrollOffset(scrollable_x, scrollable_y));
  SnapAreaData expected_area(SnapAxis::kBoth,
                             gfx::ScrollOffset(snap_offset_x, snap_offset_y),
                             must_snap);
  expected_container.AddSnapAreaData(expected_area);

  CHECK_CONTAINER(actual_container, expected_container);
  CHECK_AREA(actual_container.snap_area_list[0], expected_area);
}

TEST_P(SnapCoordinatorTest, OverflowedSnapPositionCalculation) {
  SetUpSingleSnapArea();
  Element* area_element = GetDocument().getElementById("area");
  area_element->setAttribute(styleAttr,
                             "left: 0px; top: 0px; scroll-snap-align: end;");
  GetDocument().UpdateStyleAndLayout();
  Element* scroller_element = GetDocument().getElementById("scroller");
  SnapCoordinator* snap_coordinator = GetDocument().GetSnapCoordinator();
  SnapContainerData actual_container =
      snap_coordinator->CalculateSnapContainerData(
          scroller_element->GetLayoutBox());

  // #container.width - #scroller.width
  double scrollable_x = 500 - 140;

  // #container.height - #scroller.height
  double scrollable_y = 500 - 160;

  // (#area.right + #area.scroll-snap-margin)
  //  - (#scroller.right - #scroller.scroll-padding)
  // = (100 + 8) - (140 - 10)
  // As scrollOffset cannot be set to -22, we set it to 0.
  double snap_offset_x = 0;

  // (#area.bottom + #area.scroll-snap-margin)
  //  - (#scroller.bottom - #scroller.scroll-padding)
  // = (100 + 8) - (160 - 10)
  // As scrollOffset cannot be set to -42, we set it to 0.
  double snap_offset_y = 0;

  bool must_snap = false;

  SnapContainerData expected_container(
      ScrollSnapType(false, SnapAxis::kBoth, SnapStrictness::kMandatory),
      gfx::ScrollOffset(scrollable_x, scrollable_y));
  SnapAreaData expected_area(SnapAxis::kBoth,
                             gfx::ScrollOffset(snap_offset_x, snap_offset_y),
                             must_snap);
  expected_container.AddSnapAreaData(expected_area);

  CHECK_CONTAINER(actual_container, expected_container);
  CHECK_AREA(actual_container.snap_area_list[0], expected_area);
}

TEST_P(SnapCoordinatorTest, FindsClosestSnapOffset) {
  SnapContainerData container_data(
      ScrollSnapType(false, SnapAxis::kBoth, SnapStrictness::kMandatory),
      gfx::ScrollOffset(360, 380));
  ScrollOffset current_offset(100, 100);
  SnapAreaData snap_x_only(
      SnapAxis::kX, gfx::ScrollOffset(80, SnapAreaData::kInvalidScrollOffset),
      false);
  SnapAreaData snap_y_only(
      SnapAxis::kY, gfx::ScrollOffset(SnapAreaData::kInvalidScrollOffset, 70),
      false);
  SnapAreaData snap_on_both(SnapAxis::kBoth, gfx::ScrollOffset(50, 150), false);
  container_data.AddSnapAreaData(snap_x_only);
  container_data.AddSnapAreaData(snap_y_only);
  container_data.AddSnapAreaData(snap_on_both);
  ScrollOffset snapped_offset =
      SnapCoordinator::FindSnapOffset(current_offset, container_data);
  ASSERT_EQ(80, snapped_offset.Width());
  ASSERT_EQ(70, snapped_offset.Height());
}

}  // namespace
