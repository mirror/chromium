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
        height: 120px;
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

#define CHECK_CONTAINER(c1, c2)                                        \
  {                                                                    \
    ASSERT_EQ(c1.scrollable_x, (c2).scrollable_x);                     \
    ASSERT_EQ((c1).scrollable_y, (c2).scrollable_y);                   \
    ASSERT_EQ((c1).scroll_snap_type, (c2).scroll_snap_type);           \
    ASSERT_EQ((c1).snap_area_list.size(), (c2).snap_area_list.size()); \
  }

#define CHECK_AREA(a1, a2)                             \
  {                                                    \
    ASSERT_EQ(a1.snap_start_x, a2.snap_start_x);       \
    ASSERT_EQ(a1.snap_start_y, a2.snap_start_y);       \
    ASSERT_EQ(a1.snap_end_x, a2.snap_end_x);           \
    ASSERT_EQ(a1.snap_end_y, a2.snap_end_y);           \
    ASSERT_EQ(a1.visible_start_x, a2.visible_start_x); \
    ASSERT_EQ(a1.visible_start_y, a2.visible_start_y); \
    ASSERT_EQ(a1.visible_end_x, a2.visible_end_x);     \
    ASSERT_EQ(a1.visible_end_y, a2.visible_end_y);     \
    ASSERT_EQ(a1.must_snap, a1.must_snap);             \
  }

TEST_P(SnapCoordinatorTest, StartAlignmentCalculation) {
  SetUpSingleSnapArea();
  Element* snap_element = GetDocument().getElementById("area");
  snap_element->setAttribute(styleAttr, "scroll-snap-align: start;");
  GetDocument().UpdateStyleAndLayout();
  Element* scroller = GetDocument().getElementById("scroller");
  SnapCoordinator* snap_coordinator = GetDocument().GetSnapCoordinator();
  SnapContainerData container =
      snap_coordinator->GetSnapContainerData(scroller->GetLayoutBox());
  SnapContainerData expected_container(
      ScrollSnapType(false, kSnapAxisBoth, SnapStrictness::kMandatory), 360,
      380);
  SnapAreaData expected_area(182, 182, 182, 182, 70, 90, 290, 290, false);
  expected_container.AddSnapAreaData(expected_area);
  CHECK_CONTAINER(container, expected_container);
  CHECK_AREA(container.snap_area_list[0], expected_area);
}

TEST_P(SnapCoordinatorTest, CenterAlignmentCalculation) {
  SetUpSingleSnapArea();
  Element* snap_element = GetDocument().getElementById("area");
  snap_element->setAttribute(styleAttr, "scroll-snap-align: center;");
  GetDocument().UpdateStyleAndLayout();
  Element* scroller = GetDocument().getElementById("scroller");
  SnapCoordinator* snap_coordinator = GetDocument().GetSnapCoordinator();
  SnapContainerData container =
      snap_coordinator->GetSnapContainerData(scroller->GetLayoutBox());
  SnapContainerData expected_container(
      ScrollSnapType(false, kSnapAxisBoth, SnapStrictness::kMandatory), 360,
      380);
  SnapAreaData expected_area(180, 190, 180, 190, 70, 90, 290, 290, false);
  expected_container.AddSnapAreaData(expected_area);
  CHECK_CONTAINER(container, expected_container);
  CHECK_AREA(container.snap_area_list[0], expected_area);
}

TEST_P(SnapCoordinatorTest, EndAlignmentCalculation) {
  SetUpSingleSnapArea();
  Element* snap_element = GetDocument().getElementById("area");
  snap_element->setAttribute(styleAttr, "scroll-snap-align: end;");
  GetDocument().UpdateStyleAndLayout();
  Element* scroller = GetDocument().getElementById("scroller");
  SnapCoordinator* snap_coordinator = GetDocument().GetSnapCoordinator();
  SnapContainerData container =
      snap_coordinator->GetSnapContainerData(scroller->GetLayoutBox());
  SnapContainerData expected_container(
      ScrollSnapType(false, kSnapAxisBoth, SnapStrictness::kMandatory), 360,
      380);
  SnapAreaData expected_area(178, 198, 178, 198, 70, 90, 290, 290, false);
  expected_container.AddSnapAreaData(expected_area);
  CHECK_CONTAINER(container, expected_container);
  CHECK_AREA(container.snap_area_list[0], expected_area);
}

TEST_P(SnapCoordinatorTest, LargeAreaCalculation) {
  SetUpSingleSnapArea();
  Element* snap_element = GetDocument().getElementById("area");
  snap_element->setAttribute(styleAttr,
                             "width: 200px; height: 200px; left: 150px; top: "
                             "150px; scroll-snap-align: start;");
  GetDocument().UpdateStyleAndLayout();
  Element* scroller = GetDocument().getElementById("scroller");
  SnapCoordinator* snap_coordinator = GetDocument().GetSnapCoordinator();
  SnapContainerData container =
      snap_coordinator->GetSnapContainerData(scroller->GetLayoutBox());
  SnapContainerData expected_container(
      ScrollSnapType(false, kSnapAxisBoth, SnapStrictness::kMandatory), 360,
      380);
  SnapAreaData expected_area(132, 132, 228, 248, 20, 40, 340, 340, false);
  expected_container.AddSnapAreaData(expected_area);
  CHECK_CONTAINER(container, expected_container);
  CHECK_AREA(container.snap_area_list[0], expected_area);
}

TEST_P(SnapCoordinatorTest, OverflowedSnapPositionCalculation) {
  SetUpSingleSnapArea();
  Element* snap_element = GetDocument().getElementById("area");
  snap_element->setAttribute(styleAttr,
                             "left: 0px; top: 0px; scroll-snap-align: end;");
  GetDocument().UpdateStyleAndLayout();
  Element* scroller = GetDocument().getElementById("scroller");
  SnapCoordinator* snap_coordinator = GetDocument().GetSnapCoordinator();
  SnapContainerData container =
      snap_coordinator->GetSnapContainerData(scroller->GetLayoutBox());
  SnapContainerData expected_container(
      ScrollSnapType(false, kSnapAxisBoth, SnapStrictness::kMandatory), 360,
      380);
  SnapAreaData expected_area(0, 0, 0, 0, 0, 0, 90, 90, false);
  expected_container.AddSnapAreaData(expected_area);
  CHECK_CONTAINER(container, expected_container);
  CHECK_AREA(container.snap_area_list[0], expected_area);
}

TEST_P(SnapCoordinatorTest, FindsClosestVisibleSnapOffsetOnX) {
  SnapContainerData container_data(
      ScrollSnapType(false, kSnapAxisX, SnapStrictness::kMandatory), 360, 380);
  ScrollOffset current_offset(100, 20);
  SnapAreaData distance_10_invisible(90, 200, 90, 200, 0, 150, 180, 250, false);
  SnapAreaData distance_20_visible(120, 40, 140, 40, 50, 0, 190, 80, false);
  SnapAreaData distance_30_visible(70, 40, 70, 40, 20, 0, 120, 80, false);
  container_data.AddSnapAreaData(distance_10_invisible);
  container_data.AddSnapAreaData(distance_20_visible);
  container_data.AddSnapAreaData(distance_30_visible);
  ScrollOffset snapped_offset =
      SnapCoordinator::FindSnapOffsetOnX(current_offset, container_data);
  ASSERT_EQ(120, snapped_offset.Width());
  ASSERT_EQ(20, snapped_offset.Height());
}

TEST_P(SnapCoordinatorTest, FindsClosestVisibleSnapOffsetOnY) {
  SnapContainerData container_data(
      ScrollSnapType(false, kSnapAxisX, SnapStrictness::kMandatory), 380, 360);
  ScrollOffset current_offset(20, 100);
  SnapAreaData distance_10_invisible(200, 90, 200, 90, 150, 0, 250, 180, false);
  SnapAreaData distance_20_visible(40, 120, 40, 140, 0, 50, 80, 190, false);
  SnapAreaData distance_30_visible(40, 70, 40, 70, 0, 20, 80, 120, false);
  container_data.AddSnapAreaData(distance_10_invisible);
  container_data.AddSnapAreaData(distance_20_visible);
  container_data.AddSnapAreaData(distance_30_visible);
  ScrollOffset snapped_offset =
      SnapCoordinator::FindSnapOffsetOnY(current_offset, container_data);
  ASSERT_EQ(20, snapped_offset.Width());
  ASSERT_EQ(120, snapped_offset.Height());
}

TEST_P(SnapCoordinatorTest, FindsClosestVisibleSnapOffsetOnBoth) {
  SnapContainerData container_data(
      ScrollSnapType(false, kSnapAxisBoth, SnapStrictness::kMandatory), 380,
      360);
  ScrollOffset current_offset(100, 100);
  SnapAreaData distance_10(110, 90, 110, 90, 70, 60, 150, 120, false);
  SnapAreaData distance_20(80, 80, 80, 80, 30, 50, 130, 110, false);
  SnapAreaData distance_30(60, 130, 70, 130, 0, 40, 130, 100, false);
  container_data.AddSnapAreaData(distance_10);
  container_data.AddSnapAreaData(distance_20);
  container_data.AddSnapAreaData(distance_30);
  ScrollOffset snapped_offset =
      SnapCoordinator::FindSnapOffsetOnBoth(current_offset, container_data);
  ASSERT_EQ(110, snapped_offset.Width());
  ASSERT_EQ(90, snapped_offset.Height());
}

TEST_P(SnapCoordinatorTest, ProximityDoesNotSnapToCurrentlyInvisibleArea) {
  SnapContainerData container_data(
      ScrollSnapType(false, kSnapAxisBoth, SnapStrictness::kProximity), 360,
      380);
  ScrollOffset current_offset(100, 20);
  SnapAreaData out_of_y_range(90, 200, 90, 200, 0, 150, 180, 250, false);
  SnapAreaData out_of_x_range(160, 40, 160, 40, 110, 0, 190, 80, false);
  container_data.AddSnapAreaData(out_of_y_range);
  container_data.AddSnapAreaData(out_of_x_range);
  ScrollOffset snapped_offset =
      SnapCoordinator::FindSnapOffsetOnBoth(current_offset, container_data);
  ASSERT_EQ(100, snapped_offset.Width());
  ASSERT_EQ(20, snapped_offset.Height());
}

TEST_P(SnapCoordinatorTest, AllowsSnappingAnywhereInLargeArea) {
  SnapContainerData container_data(
      ScrollSnapType(false, kSnapAxisBoth, SnapStrictness::kProximity), 360,
      380);
  ScrollOffset current_offset(100, 120);
  SnapAreaData large_area(90, 100, 130, 200, 40, 100, 170, 230, false);
  container_data.AddSnapAreaData(large_area);
  ScrollOffset snapped_offset =
      SnapCoordinator::FindSnapOffsetOnBoth(current_offset, container_data);
  ASSERT_EQ(100, snapped_offset.Width());
  ASSERT_EQ(120, snapped_offset.Height());
}

}  // namespace
