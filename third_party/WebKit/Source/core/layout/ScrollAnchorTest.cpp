// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/layout/ScrollAnchor.h"

#include "core/dom/ClientRect.h"
#include "core/frame/VisualViewport.h"
#include "core/layout/LayoutBox.h"
#include "core/layout/LayoutTestHelper.h"
#include "core/page/PrintContext.h"
#include "core/paint/PaintLayerScrollableArea.h"
#include "platform/testing/HistogramTester.h"
#include "platform/testing/RuntimeEnabledFeaturesTestHelpers.h"

namespace blink {

using Corner = ScrollAnchor::Corner;

typedef bool TestParamRootLayerScrolling;
class ScrollAnchorTest
    : public testing::WithParamInterface<TestParamRootLayerScrolling>,
      private ScopedRootLayerScrollingForTest,
      public RenderingTest {
 public:
  ScrollAnchorTest() : ScopedRootLayerScrollingForTest(GetParam()) {
    RuntimeEnabledFeatures::setScrollAnchoringEnabled(true);
  }
  ~ScrollAnchorTest() {
    RuntimeEnabledFeatures::setScrollAnchoringEnabled(false);
  }

 protected:
  void Update() {
    // TODO(skobes): Use SimTest instead of RenderingTest and move into
    // Source/web?
    GetDocument().View()->UpdateAllLifecyclePhases();
  }

  ScrollableArea* LayoutViewport() {
    return GetDocument().View()->LayoutViewportScrollableArea();
  }

  VisualViewport& GetVisualViewport() {
    return GetDocument().View()->GetPage()->GetVisualViewport();
  }

  ScrollableArea* ScrollerForElement(Element* element) {
    return ToLayoutBox(element->GetLayoutObject())->GetScrollableArea();
  }

  ScrollAnchor& GetScrollAnchor(ScrollableArea* scroller) {
    DCHECK(scroller->IsLocalFrameView() ||
           scroller->IsPaintLayerScrollableArea());
    return *(scroller->GetScrollAnchor());
  }

  void SetHeight(Element* element, int height) {
    element->setAttribute(HTMLNames::styleAttr,
                          AtomicString(String::Format("height: %dpx", height)));
    Update();
  }

  void ScrollLayoutViewport(ScrollOffset delta) {
    Element* scrolling_element = GetDocument().scrollingElement();
    if (delta.Width())
      scrolling_element->setScrollLeft(scrolling_element->scrollLeft() +
                                       delta.Width());
    if (delta.Height())
      scrolling_element->setScrollTop(scrolling_element->scrollTop() +
                                      delta.Height());
  }
};

INSTANTIATE_TEST_CASE_P(All, ScrollAnchorTest, ::testing::Bool());

// TODO(ymalik): Currently, this should be the first test in the file to avoid
// failure when running with other tests. Dig into this more and fix.
TEST_P(ScrollAnchorTest, UMAMetricUpdated) {
  HistogramTester histogram_tester;
  SetBodyInnerHTML(
      "<style> body { height: 1000px } div { height: 100px } </style>"
      "<div id='block1'>abc</div>"
      "<div id='block2'>def</div>");

  ScrollableArea* viewport = LayoutViewport();

  // Scroll position not adjusted, metric not updated.
  ScrollLayoutViewport(ScrollOffset(0, 150));
  histogram_tester.ExpectTotalCount("Layout.ScrollAnchor.AdjustedScrollOffset",
                                    0);

  // Height changed, verify metric updated once.
  SetHeight(GetDocument().getElementById("block1"), 200);
  histogram_tester.ExpectUniqueSample(
      "Layout.ScrollAnchor.AdjustedScrollOffset", 1, 1);

  EXPECT_EQ(250, viewport->ScrollOffsetInt().Height());
  EXPECT_EQ(GetDocument().getElementById("block2")->GetLayoutObject(),
            GetScrollAnchor(viewport).AnchorObject());
}

// TODO(skobes): Convert this to web-platform-tests when visual viewport API is
// launched (http://crbug.com/635031).
TEST_P(ScrollAnchorTest, VisualViewportAnchors) {
  SetBodyInnerHTML(
      "<style>"
      "    * { font-size: 1.2em; font-family: sans-serif; }"
      "    div { height: 100px; width: 20px; background-color: pink; }"
      "</style>"
      "<div id='div'></div>"
      "<div id='text'><b>This is a scroll anchoring test</div>");

  ScrollableArea* l_viewport = LayoutViewport();
  VisualViewport& v_viewport = GetVisualViewport();

  v_viewport.SetScale(2.0);

  // No anchor at origin (0,0).
  EXPECT_EQ(nullptr, GetScrollAnchor(l_viewport).AnchorObject());

  // Scroll the visual viewport to bring #text to the top.
  int top =
      GetDocument().getElementById("text")->getBoundingClientRect()->top();
  v_viewport.SetLocation(FloatPoint(0, top));

  SetHeight(GetDocument().getElementById("div"), 10);
  EXPECT_EQ(GetDocument().getElementById("text")->GetLayoutObject(),
            GetScrollAnchor(l_viewport).AnchorObject());
  EXPECT_EQ(top - 90, v_viewport.ScrollOffsetInt().Height());

  SetHeight(GetDocument().getElementById("div"), 100);
  EXPECT_EQ(GetDocument().getElementById("text")->GetLayoutObject(),
            GetScrollAnchor(l_viewport).AnchorObject());
  EXPECT_EQ(top, v_viewport.ScrollOffsetInt().Height());

  // Scrolling the visual viewport should clear the anchor.
  v_viewport.SetLocation(FloatPoint(0, 0));
  EXPECT_EQ(nullptr, GetScrollAnchor(l_viewport).AnchorObject());
}

// Test that a non-anchoring scroll on scroller clears scroll anchors for all
// parent scrollers.
TEST_P(ScrollAnchorTest, ClearScrollAnchorsOnAncestors) {
  SetBodyInnerHTML(
      "<style>"
      "    body { height: 1000px } div { height: 200px }"
      "    #scroller { height: 100px; width: 200px; overflow: scroll; }"
      "</style>"
      "<div id='changer'>abc</div>"
      "<div id='anchor'>def</div>"
      "<div id='scroller'><div></div></div>");

  ScrollableArea* viewport = LayoutViewport();

  ScrollLayoutViewport(ScrollOffset(0, 250));
  SetHeight(GetDocument().getElementById("changer"), 300);

  EXPECT_EQ(350, viewport->ScrollOffsetInt().Height());
  EXPECT_EQ(GetDocument().getElementById("anchor")->GetLayoutObject(),
            GetScrollAnchor(viewport).AnchorObject());

  // Scrolling the nested scroller should clear the anchor on the main frame.
  ScrollableArea* scroller =
      ScrollerForElement(GetDocument().getElementById("scroller"));
  scroller->ScrollBy(ScrollOffset(0, 100), kUserScroll);
  EXPECT_EQ(nullptr, GetScrollAnchor(viewport).AnchorObject());
}

TEST_P(ScrollAnchorTest, AncestorClearingWithSiblingReference) {
  SetBodyInnerHTML(
      "<style>"
      ".scroller {"
      "  overflow: scroll;"
      "  width: 400px;"
      "  height: 400px;"
      "}"
      ".space {"
      "  width: 100px;"
      "  height: 600px;"
      "}"
      "</style>"
      "<div id='s1' class='scroller'>"
      "  <div id='anchor' class='space'></div>"
      "</div>"
      "<div id='s2' class='scroller'>"
      "  <div class='space'></div>"
      "</div>");
  Element* s1 = GetDocument().getElementById("s1");
  Element* s2 = GetDocument().getElementById("s2");
  Element* anchor = GetDocument().getElementById("anchor");

  // Set non-zero scroll offsets for #s1 and #document
  s1->setScrollTop(100);
  ScrollLayoutViewport(ScrollOffset(0, 100));

  // Invalidate layout.
  SetHeight(anchor, 500);

  // This forces layout, during which both #s1 and #document will anchor to
  // #anchor. Then the scroll clears #s2 and #document.  Since #anchor is still
  // referenced by #s1, its IsScrollAnchorObject bit must remain set.
  s2->setScrollTop(100);

  // This should clear #s1.  If #anchor had its bit cleared already we would
  // crash in update().
  s1->RemoveChild(anchor);
  Update();
}

TEST_P(ScrollAnchorTest, FractionalOffsetsAreRoundedBeforeComparing) {
  SetBodyInnerHTML(
      "<style> body { height: 1000px } </style>"
      "<div id='block1' style='height: 50.4px'>abc</div>"
      "<div id='block2' style='height: 100px'>def</div>");

  ScrollableArea* viewport = LayoutViewport();
  ScrollLayoutViewport(ScrollOffset(0, 100));

  GetDocument().getElementById("block1")->setAttribute(HTMLNames::styleAttr,
                                                       "height: 50.6px");
  Update();

  EXPECT_EQ(101, viewport->ScrollOffsetInt().Height());
}

TEST_P(ScrollAnchorTest, AnchorWithLayerInScrollingDiv) {
  SetBodyInnerHTML(
      "<style>"
      "    #scroller { overflow: scroll; width: 500px; height: 400px; }"
      "    div { height: 100px }"
      "    #block2 { overflow: hidden }"
      "    #space { height: 1000px; }"
      "</style>"
      "<div id='scroller'><div id='space'>"
      "<div id='block1'>abc</div>"
      "<div id='block2'>def</div>"
      "</div></div>");

  ScrollableArea* scroller =
      ScrollerForElement(GetDocument().getElementById("scroller"));
  Element* block1 = GetDocument().getElementById("block1");
  Element* block2 = GetDocument().getElementById("block2");

  scroller->ScrollBy(ScrollOffset(0, 150), kUserScroll);

  // In this layout pass we will anchor to #block2 which has its own PaintLayer.
  SetHeight(block1, 200);
  EXPECT_EQ(250, scroller->ScrollOffsetInt().Height());
  EXPECT_EQ(block2->GetLayoutObject(),
            GetScrollAnchor(scroller).AnchorObject());

  // Test that the anchor object can be destroyed without affecting the scroll
  // position.
  block2->remove();
  Update();
  EXPECT_EQ(250, scroller->ScrollOffsetInt().Height());
}

// Verify that a nested scroller with a div that has its own PaintLayer can be
// removed without causing a crash. This test passes if it doesn't crash.
TEST_P(ScrollAnchorTest, RemoveScrollerWithLayerInScrollingDiv) {
  SetBodyInnerHTML(
      "<style>"
      "    body { height: 2000px }"
      "    #scroller { overflow: scroll; width: 500px; height: 400px}"
      "    #block1 { height: 100px; width: 100px; overflow: hidden}"
      "    #anchor { height: 1000px; }"
      "</style>"
      "<div id='changer1'></div>"
      "<div id='scroller'>"
      "  <div id='changer2'></div>"
      "  <div id='block1'></div>"
      "  <div id='anchor'></div>"
      "</div>");

  ScrollableArea* viewport = LayoutViewport();
  ScrollableArea* scroller =
      ScrollerForElement(GetDocument().getElementById("scroller"));
  Element* changer1 = GetDocument().getElementById("changer1");
  Element* changer2 = GetDocument().getElementById("changer2");
  Element* anchor = GetDocument().getElementById("anchor");

  scroller->ScrollBy(ScrollOffset(0, 150), kUserScroll);
  ScrollLayoutViewport(ScrollOffset(0, 50));

  // In this layout pass both the inner and outer scroller will anchor to
  // #anchor.
  SetHeight(changer1, 100);
  SetHeight(changer2, 100);
  EXPECT_EQ(250, scroller->ScrollOffsetInt().Height());
  EXPECT_EQ(anchor->GetLayoutObject(),
            GetScrollAnchor(scroller).AnchorObject());
  EXPECT_EQ(anchor->GetLayoutObject(),
            GetScrollAnchor(viewport).AnchorObject());

  // Test that the inner scroller can be destroyed without crashing.
  GetDocument().getElementById("scroller")->remove();
  Update();
}

TEST_P(ScrollAnchorTest, FlexboxDelayedClampingAlsoDelaysAdjustment) {
  SetBodyInnerHTML(
      "<style>"
      "    html { overflow: hidden; }"
      "    body {"
      "        position: absolute; display: flex;"
      "        top: 0; bottom: 0; margin: 0;"
      "    }"
      "    #scroller { overflow: auto; }"
      "    #spacer { width: 600px; height: 1200px; }"
      "    #before { height: 50px; }"
      "    #anchor {"
      "        width: 100px; height: 100px;"
      "        background-color: #8f8;"
      "    }"
      "</style>"
      "<div id='scroller'>"
      "    <div id='spacer'>"
      "        <div id='before'></div>"
      "        <div id='anchor'></div>"
      "    </div>"
      "</div>");

  Element* scroller = GetDocument().getElementById("scroller");
  scroller->setScrollTop(100);

  SetHeight(GetDocument().getElementById("before"), 100);
  EXPECT_EQ(150, ScrollerForElement(scroller)->ScrollOffsetInt().Height());
}

TEST_P(ScrollAnchorTest, FlexboxDelayedAdjustmentRespectsSANACLAP) {
  SetBodyInnerHTML(
      "<style>"
      "    html { overflow: hidden; }"
      "    body {"
      "        position: absolute; display: flex;"
      "        top: 0; bottom: 0; margin: 0;"
      "    }"
      "    #scroller { overflow: auto; }"
      "    #spacer { width: 600px; height: 1200px; }"
      "    #anchor {"
      "        position: relative; top: 50px;"
      "        width: 100px; height: 100px;"
      "        background-color: #8f8;"
      "    }"
      "</style>"
      "<div id='scroller'>"
      "    <div id='spacer'>"
      "        <div id='anchor'></div>"
      "    </div>"
      "</div>");

  Element* scroller = GetDocument().getElementById("scroller");
  scroller->setScrollTop(100);

  GetDocument().getElementById("spacer")->setAttribute(HTMLNames::styleAttr,
                                                       "margin-top: 50px");
  Update();
  EXPECT_EQ(100, ScrollerForElement(scroller)->ScrollOffsetInt().Height());
}

TEST_P(ScrollAnchorTest, NonDefaultRootScroller) {
  SetBodyInnerHTML(
      "<style>"
      "    ::-webkit-scrollbar {"
      "      width: 0px; height: 0px;"
      "    }"
      "    body, html {"
      "      margin: 0px; width: 100%; height: 100%;"
      "    }"
      "    #rootscroller {"
      "      overflow: scroll; width: 100%; height: 100%;"
      "    }"
      "    .spacer {"
      "      height: 600px; width: 100px;"
      "    }"
      "    #target {"
      "      height: 100px; width: 100px; background-color: red;"
      "    }"
      "</style>"
      "<div id='rootscroller'>"
      "    <div id='firstChild' class='spacer'></div>"
      "    <div id='target'></div>"
      "    <div class='spacer'></div>"
      "</div>"
      "<div class='spacer'></div>");

  Element* root_scroller_element = GetDocument().getElementById("rootscroller");

  NonThrowableExceptionState non_throw;
  GetDocument().setRootScroller(root_scroller_element, non_throw);
  GetDocument().View()->UpdateAllLifecyclePhases();

  ScrollableArea* scroller = ScrollerForElement(root_scroller_element);

  // By making the #rootScroller DIV the rootScroller, it should become the
  // layout viewport on the RootFrameViewport.
  ASSERT_EQ(scroller,
            &GetDocument().View()->GetRootFrameViewport()->LayoutViewport());

  // The #rootScroller DIV's anchor should have the RootFrameViewport set as
  // the scroller, rather than the FrameView's anchor.

  root_scroller_element->setScrollTop(600);

  SetHeight(GetDocument().getElementById("firstChild"), 1000);

  // Scroll anchoring should be applied to #rootScroller.
  EXPECT_EQ(1000, scroller->GetScrollOffset().Height());
  EXPECT_EQ(GetDocument().getElementById("target")->GetLayoutObject(),
            GetScrollAnchor(scroller).AnchorObject());
  // Scroll anchoring should not apply within main frame.
  EXPECT_EQ(0, LayoutViewport()->GetScrollOffset().Height());
  EXPECT_EQ(nullptr, GetScrollAnchor(LayoutViewport()).AnchorObject());
}

// This test verifies that scroll anchoring is disabled when the document is in
// printing mode.
TEST_P(ScrollAnchorTest, AnchoringDisabledForPrinting) {
  SetBodyInnerHTML(
      "<style> body { height: 1000px } div { height: 100px } </style>"
      "<div id='block1'>abc</div>"
      "<div id='block2'>def</div>");

  ScrollableArea* viewport = LayoutViewport();
  ScrollLayoutViewport(ScrollOffset(0, 150));

  // This will trigger printing and layout.
  PrintContext::NumberOfPages(GetDocument().GetFrame(), FloatSize(500, 500));

  EXPECT_EQ(150, viewport->ScrollOffsetInt().Height());
  EXPECT_EQ(nullptr, GetScrollAnchor(viewport).AnchorObject());
}

//////////////////////////////////////////////////////////////////////////////
// TODO(skobes): Convert tests below this line to WPT.
// Tests above this line will remain in ScrollAnchorTest.cpp to implementation-
// specific aspects of scroll anchoring (anchor node caching mechanism,
// PaintLayer interactions, etc).
//////////////////////////////////////////////////////////////////////////////

// Test then an element and its children are not selected as the anchor when
// it has the overflow-anchor property set to none.
TEST_P(ScrollAnchorTest, OptOutElement) {
  SetBodyInnerHTML(
      "<style>"
      "     body { height: 1000px }"
      "     .div {"
      "          height: 100px; width: 100px;"
      "          border: 1px solid gray; background-color: #afa;"
      "     }"
      "     #innerDiv {"
      "          height: 50px; width: 50px;"
      "          border: 1px solid gray; background-color: pink;"
      "     }"
      "</style>"
      "<div id='changer'></div>"
      "<div class='div' id='firstDiv'><div id='innerDiv'></div></div>"
      "<div class='div' id='secondDiv'></div>");

  ScrollableArea* viewport = LayoutViewport();
  ScrollLayoutViewport(ScrollOffset(0, 50));

  // No opt-out.
  SetHeight(GetDocument().getElementById("changer"), 100);
  EXPECT_EQ(150, viewport->ScrollOffsetInt().Height());
  EXPECT_EQ(GetDocument().getElementById("innerDiv")->GetLayoutObject(),
            GetScrollAnchor(viewport).AnchorObject());

  // Clear anchor and opt-out element.
  ScrollLayoutViewport(ScrollOffset(0, 10));
  GetDocument()
      .getElementById("firstDiv")
      ->setAttribute(HTMLNames::styleAttr,
                     AtomicString("overflow-anchor: none"));
  Update();

  // Opted out element and it's children skipped.
  SetHeight(GetDocument().getElementById("changer"), 200);
  EXPECT_EQ(260, viewport->ScrollOffsetInt().Height());
  EXPECT_EQ(GetDocument().getElementById("secondDiv")->GetLayoutObject(),
            GetScrollAnchor(viewport).AnchorObject());
}

TEST_P(ScrollAnchorTest, OptOutBody) {
  SetBodyInnerHTML(
      "<style>"
      "    body { height: 2000px; overflow-anchor: none; }"
      "    #scroller { overflow: scroll; width: 500px; height: 300px; }"
      "    .anchor {"
      "        position:relative; height: 100px; width: 150px;"
      "        background-color: #afa; border: 1px solid gray;"
      "    }"
      "    #forceScrolling { height: 500px; background-color: #fcc; }"
      "</style>"
      "<div id='outerChanger'></div>"
      "<div id='outerAnchor' class='anchor'></div>"
      "<div id='scroller'>"
      "    <div id='innerChanger'></div>"
      "    <div id='innerAnchor' class='anchor'></div>"
      "    <div id='forceScrolling'></div>"
      "</div>");

  ScrollableArea* scroller =
      ScrollerForElement(GetDocument().getElementById("scroller"));
  ScrollableArea* viewport = LayoutViewport();

  GetDocument().getElementById("scroller")->setScrollTop(100);
  ScrollLayoutViewport(ScrollOffset(0, 100));

  SetHeight(GetDocument().getElementById("innerChanger"), 200);
  SetHeight(GetDocument().getElementById("outerChanger"), 150);

  // Scroll anchoring should apply within #scroller.
  EXPECT_EQ(300, scroller->ScrollOffsetInt().Height());
  EXPECT_EQ(GetDocument().getElementById("innerAnchor")->GetLayoutObject(),
            GetScrollAnchor(scroller).AnchorObject());
  // Scroll anchoring should not apply within main frame.
  EXPECT_EQ(100, viewport->ScrollOffsetInt().Height());
  EXPECT_EQ(nullptr, GetScrollAnchor(viewport).AnchorObject());
}

TEST_P(ScrollAnchorTest, OptOutScrollingDiv) {
  SetBodyInnerHTML(
      "<style>"
      "    body { height: 2000px; }"
      "    #scroller {"
      "        overflow: scroll; width: 500px; height: 300px;"
      "        overflow-anchor: none;"
      "    }"
      "    .anchor {"
      "        position:relative; height: 100px; width: 150px;"
      "        background-color: #afa; border: 1px solid gray;"
      "    }"
      "    #forceScrolling { height: 500px; background-color: #fcc; }"
      "</style>"
      "<div id='outerChanger'></div>"
      "<div id='outerAnchor' class='anchor'></div>"
      "<div id='scroller'>"
      "    <div id='innerChanger'></div>"
      "    <div id='innerAnchor' class='anchor'></div>"
      "    <div id='forceScrolling'></div>"
      "</div>");

  ScrollableArea* scroller =
      ScrollerForElement(GetDocument().getElementById("scroller"));
  ScrollableArea* viewport = LayoutViewport();

  GetDocument().getElementById("scroller")->setScrollTop(100);
  ScrollLayoutViewport(ScrollOffset(0, 100));

  SetHeight(GetDocument().getElementById("innerChanger"), 200);
  SetHeight(GetDocument().getElementById("outerChanger"), 150);

  // Scroll anchoring should not apply within #scroller.
  EXPECT_EQ(100, scroller->ScrollOffsetInt().Height());
  EXPECT_EQ(nullptr, GetScrollAnchor(scroller).AnchorObject());
  // Scroll anchoring should apply within main frame.
  EXPECT_EQ(250, viewport->ScrollOffsetInt().Height());
  EXPECT_EQ(GetDocument().getElementById("outerAnchor")->GetLayoutObject(),
            GetScrollAnchor(viewport).AnchorObject());
}

class ScrollAnchorCornerTest : public ScrollAnchorTest {
 protected:
  void CheckCorner(Corner corner,
                   ScrollOffset start_offset,
                   ScrollOffset expected_adjustment) {
    ScrollableArea* viewport = LayoutViewport();
    Element* element = GetDocument().getElementById("changer");

    viewport->SetScrollOffset(start_offset, kUserScroll);
    element->setAttribute(HTMLNames::classAttr, "change");
    Update();

    ScrollOffset end_pos = start_offset;
    end_pos += expected_adjustment;

    EXPECT_EQ(end_pos, viewport->GetScrollOffset());
    EXPECT_EQ(GetDocument().getElementById("a")->GetLayoutObject(),
              GetScrollAnchor(viewport).AnchorObject());
    EXPECT_EQ(corner, GetScrollAnchor(viewport).GetCorner());

    element->removeAttribute(HTMLNames::classAttr);
    Update();
  }
};

// Verify that we anchor to the top left corner of an element for LTR.
TEST_P(ScrollAnchorCornerTest, CornersLTR) {
  SetBodyInnerHTML(
      "<style>"
      "    body { position: relative; width: 1220px; height: 920px; }"
      "    #a { width: 400px; height: 300px; }"
      "    .change { height: 100px; }"
      "</style>"
      "<div id='changer'></div>"
      "<div id='a'></div>");

  CheckCorner(Corner::kTopLeft, ScrollOffset(20, 20), ScrollOffset(0, 100));
}

// Verify that we anchor to the top left corner of an anchor element for
// vertical-lr writing mode.
TEST_P(ScrollAnchorCornerTest, CornersVerticalLR) {
  SetBodyInnerHTML(
      "<style>"
      "    html { writing-mode: vertical-lr; }"
      "    body { position: relative; width: 1220px; height: 920px; }"
      "    #a { width: 400px; height: 300px; }"
      "    .change { width: 100px; }"
      "</style>"
      "<div id='changer'></div>"
      "<div id='a'></div>");

  CheckCorner(Corner::kTopLeft, ScrollOffset(20, 20), ScrollOffset(100, 0));
}

// Verify that we anchor to the top right corner of an anchor element for RTL.
TEST_P(ScrollAnchorCornerTest, CornersRTL) {
  SetBodyInnerHTML(
      "<style>"
      "    html { direction: rtl; }"
      "    body { position: relative; width: 1220px; height: 920px; }"
      "    #a { width: 400px; height: 300px; }"
      "    .change { height: 100px; }"
      "</style>"
      "<div id='changer'></div>"
      "<div id='a'></div>");

  CheckCorner(Corner::kTopRight, ScrollOffset(-20, 20), ScrollOffset(0, 100));
}

// Verify that we anchor to the top right corner of an anchor element for
// vertical-lr writing mode.
TEST_P(ScrollAnchorCornerTest, CornersVerticalRL) {
  SetBodyInnerHTML(
      "<style>"
      "    html { writing-mode: vertical-rl; }"
      "    body { position: relative; width: 1220px; height: 920px; }"
      "    #a { width: 400px; height: 300px; }"
      "    .change { width: 100px; }"
      "</style>"
      "<div id='changer'></div>"
      "<div id='a'></div>");

  CheckCorner(Corner::kTopRight, ScrollOffset(-20, 20), ScrollOffset(-100, 0));
}

TEST_P(ScrollAnchorTest, IgnoreNonBlockLayoutAxis) {
  SetBodyInnerHTML(
      "<style>"
      "    body {"
      "        margin: 0; line-height: 0;"
      "        width: 1200px; height: 1200px;"
      "    }"
      "    div {"
      "        width: 100px; height: 100px;"
      "        border: 5px solid gray;"
      "        display: inline-block;"
      "        box-sizing: border-box;"
      "    }"
      "</style>"
      "<div id='a'></div><br>"
      "<div id='b'></div><div id='c'></div>");

  ScrollableArea* viewport = LayoutViewport();
  ScrollLayoutViewport(ScrollOffset(150, 0));

  Element* a = GetDocument().getElementById("a");
  Element* b = GetDocument().getElementById("b");
  Element* c = GetDocument().getElementById("c");

  a->setAttribute(HTMLNames::styleAttr, "height: 150px");
  Update();
  EXPECT_EQ(ScrollOffset(150, 0), viewport->GetScrollOffset());
  EXPECT_EQ(nullptr, GetScrollAnchor(viewport).AnchorObject());

  ScrollLayoutViewport(ScrollOffset(0, 50));

  a->setAttribute(HTMLNames::styleAttr, "height: 200px");
  b->setAttribute(HTMLNames::styleAttr, "width: 150px");
  Update();
  EXPECT_EQ(ScrollOffset(150, 100), viewport->GetScrollOffset());
  EXPECT_EQ(c->GetLayoutObject(), GetScrollAnchor(viewport).AnchorObject());

  a->setAttribute(HTMLNames::styleAttr, "height: 100px");
  b->setAttribute(HTMLNames::styleAttr, "width: 100px");
  GetDocument().documentElement()->setAttribute(HTMLNames::styleAttr,
                                                "writing-mode: vertical-rl");
  GetDocument().scrollingElement()->setScrollLeft(0);
  GetDocument().scrollingElement()->setScrollTop(0);
  ScrollLayoutViewport(ScrollOffset(0, 150));

  a->setAttribute(HTMLNames::styleAttr, "width: 150px");
  Update();
  EXPECT_EQ(ScrollOffset(0, 150), viewport->GetScrollOffset());
  EXPECT_EQ(nullptr, GetScrollAnchor(viewport).AnchorObject());

  ScrollLayoutViewport(ScrollOffset(-50, 0));

  a->setAttribute(HTMLNames::styleAttr, "width: 200px");
  b->setAttribute(HTMLNames::styleAttr, "height: 150px");
  Update();
  EXPECT_EQ(ScrollOffset(-100, 150), viewport->GetScrollOffset());
  EXPECT_EQ(c->GetLayoutObject(), GetScrollAnchor(viewport).AnchorObject());
}
}
