// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/paint/ViewPainter.h"

#include <gtest/gtest.h>
#include "core/paint/PaintControllerPaintTest.h"
#include "core/paint/compositing/CompositedLayerMapping.h"
#include "platform/graphics/paint/DrawingDisplayItem.h"

namespace blink {

class ViewPainterTest : public ::testing::WithParamInterface<bool>,
                        private ScopedRootLayerScrollingForTest,
                        public PaintControllerPaintTestBase {
 public:
  ViewPainterTest()
      : ScopedRootLayerScrollingForTest(GetParam()),
        PaintControllerPaintTestBase(false) {}

 protected:
  void RunFixedBackgroundTest(bool high_dpi);
};

INSTANTIATE_TEST_CASE_P(All, ViewPainterTest, ::testing::Bool());

void ViewPainterTest::RunFixedBackgroundTest(bool high_dpi) {
  if (high_dpi) {
    Settings* settings = GetDocument().GetFrame()->GetSettings();
    settings->SetPreferCompositingToLCDTextEnabled(true);
  }
  SetBodyInnerHTML(
      "<style>"
      "  ::-webkit-scrollbar { display: none; }"
      "  body {"
      "    margin: 0;"
      "    width: 1200px;"
      "    height: 900px;"
      "    background: radial-gradient("
      "      circle at 100px 100px, blue, transparent 200px) fixed;"
      "  }"
      "</style>");

  LocalFrameView* frame_view = GetDocument().View();
  ScrollableArea* layout_viewport = frame_view->LayoutViewportScrollableArea();

  ScrollOffset scroll_offset(200, 150);
  layout_viewport->SetScrollOffset(scroll_offset, kUserScroll);
  frame_view->UpdateAllLifecyclePhases();

  bool rls = RuntimeEnabledFeatures::RootLayerScrollingEnabled();
  CompositedLayerMapping* clm =
      GetLayoutView().Layer()->GetCompositedLayerMapping();
  GraphicsLayer* layer_for_background;
  if (high_dpi) {
    layer_for_background =
        rls ? clm->MainGraphicsLayer() : clm->BackgroundLayer();
  } else {
    layer_for_background =
        rls ? clm->ScrollingContentsLayer() : clm->MainGraphicsLayer();
  }
  const DisplayItemList& display_items =
      layer_for_background->GetPaintController().GetDisplayItemList();
  const DisplayItem& background = display_items[rls && !high_dpi ? 2 : 0];
  EXPECT_EQ(background.GetType(), kDocumentBackgroundType);
  DisplayItemClient* expected_client;
  if (rls && !high_dpi)
    expected_client = GetLayoutView().Layer()->GraphicsLayerBacking();
  else
    expected_client = &GetLayoutView();
  EXPECT_EQ(&background.Client(), expected_client);

  sk_sp<const PaintRecord> record =
      static_cast<const DrawingDisplayItem&>(background).GetPaintRecord();
  ASSERT_EQ(record->size(), 2u);
  cc::PaintOpBuffer::Iterator it(record.get());
  ASSERT_EQ((*++it)->GetType(), cc::PaintOpType::DrawRect);

  // This is the dest_rect_ calculated by BackgroundImageGeometry.  For a fixed
  // background in scrolling contents layer, its location is the scroll offset.
  SkRect rect = static_cast<const cc::DrawRectOp*>(*it)->rect;
  ASSERT_EQ(high_dpi ? ScrollOffset() : scroll_offset,
            ScrollOffset(rect.fLeft, rect.fTop));
}

TEST_P(ViewPainterTest, DocumentFixedBackgroundLowDPI) {
  RunFixedBackgroundTest(false);
}

TEST_P(ViewPainterTest, DocumentFixedBackgroundHighDPI) {
  RunFixedBackgroundTest(true);
}

}  // namespace blink
