// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/paint/ng/ng_paint_fragment.h"

#include "core/layout/LayoutTestHelper.h"
#include "core/layout/ng/inline/ng_physical_line_box_fragment.h"
#include "core/layout/ng/inline/ng_physical_text_fragment.h"
#include "core/layout/ng/layout_ng_block_flow.h"
#include "core/layout/ng/ng_block_node.h"
#include "core/paint/PaintControllerPaintTest.h"
#include "core/paint/PaintInfo.h"
#include "core/paint/PaintLayerPainter.h"
#include "core/style/ComputedStyle.h"
#include "platform/graphics/GraphicsContext.h"
#include "platform/graphics/paint/PaintController.h"
#include "platform/runtime_enabled_features.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace blink {

class NGPaintFragmentTest : public RenderingTest,
                            private ScopedLayoutNGForTest,
                            private ScopedLayoutNGPaintFragmentsForTest {
 public:
  NGPaintFragmentTest(LocalFrameClient* local_frame_client = nullptr)
      : RenderingTest(local_frame_client),
        ScopedLayoutNGForTest(true),
        ScopedLayoutNGPaintFragmentsForTest(true) {}
};

TEST_F(NGPaintFragmentTest, VisualRectForInlineBox) {
  LoadAhem();
  SetBodyInnerHTML(R"HTML(
    <!DOCTYPE html>
    <style>
    html, body { margin: 0; }
    div { font: 10px Ahem; }
    </style>
    <body>
      <div id="container">1234567890<span id="box">XXX<span></div>
    </body>
  )HTML");

  const LayoutNGBlockFlow& block_flow =
      ToLayoutNGBlockFlow(*GetLayoutObjectByElementId("container"));

  const NGPaintFragment& root_fragment = *block_flow.PaintFragment();
  EXPECT_EQ(1u, root_fragment.Children().size());
  const NGPaintFragment& line_box_fragment = *root_fragment.Children()[0];
  EXPECT_EQ(NGPhysicalFragment::kFragmentLineBox,
            line_box_fragment.PhysicalFragment().Type());
  EXPECT_EQ(2u, line_box_fragment.Children().size());

  // Inline boxes without box decorations (border, background, etc.) do not
  // generate box fragments and that their child fragments are placed directly
  // under the line box.
  const NGPaintFragment& text_fragment = *line_box_fragment.Children()[1];
  EXPECT_EQ(NGPhysicalFragment::kFragmentText,
            text_fragment.PhysicalFragment().Type());
  EXPECT_EQ(LayoutRect(100, 0, 30, 10), text_fragment.VisualRect());
}

TEST_F(NGPaintFragmentTest, VisualRectForInlineBoxWithDecorations) {
  LoadAhem();
  SetBodyInnerHTML(R"HTML(
    <!DOCTYPE html>
    <style>
    html, body { margin: 0; }
    div { font: 10px Ahem; }
    span { background: blue; }
    </style>
    <body>
      <div id="container">1234567890<span id="box">XXX<span></div>
    </body>
  )HTML");

  const LayoutNGBlockFlow& block_flow =
      ToLayoutNGBlockFlow(*GetLayoutObjectByElementId("container"));

  const NGPaintFragment& root_fragment = *block_flow.PaintFragment();
  EXPECT_EQ(1u, root_fragment.Children().size());
  const NGPaintFragment& line_box_fragment = *root_fragment.Children()[0];
  EXPECT_EQ(NGPhysicalFragment::kFragmentLineBox,
            line_box_fragment.PhysicalFragment().Type());
  EXPECT_EQ(2u, line_box_fragment.Children().size());

  // Inline boxes with box decorations generate box fragments.
  const NGPaintFragment& box_fragment = *line_box_fragment.Children()[1];
  EXPECT_EQ(NGPhysicalFragment::kFragmentBox,
            box_fragment.PhysicalFragment().Type());
  EXPECT_EQ(LayoutRect(100, 0, 30, 10), box_fragment.VisualRect());

  EXPECT_EQ(1u, box_fragment.Children().size());
  const NGPaintFragment& text_fragment = *box_fragment.Children()[0];
  EXPECT_EQ(NGPhysicalFragment::kFragmentText,
            text_fragment.PhysicalFragment().Type());

  // VisualRect of descendants of inline boxes are relative to its inline
  // formatting context, not to its parent inline box.
  EXPECT_EQ(LayoutRect(100, 0, 30, 10), text_fragment.VisualRect());
}

TEST_F(NGPaintFragmentTest, DISABLED_VisualRectForInlineBoxWithRelative) {
  LoadAhem();
  SetBodyInnerHTML(R"HTML(
    <!DOCTYPE html>
    <style>
    html, body { margin: 0; }
    div { font: 10px Ahem; }
    span { position: relative; top: 10px; }
    </style>
    <body>
      <div id="container">1234567890<span id="box">XXX<span></div>
    </body>
  )HTML");

  const LayoutNGBlockFlow& block_flow =
      ToLayoutNGBlockFlow(*GetLayoutObjectByElementId("container"));

  const NGPaintFragment& root_fragment = *block_flow.PaintFragment();
  EXPECT_EQ(1u, root_fragment.Children().size());
  const NGPaintFragment& line_box_fragment = *root_fragment.Children()[0];
  EXPECT_EQ(NGPhysicalFragment::kFragmentLineBox,
            line_box_fragment.PhysicalFragment().Type());
  EXPECT_EQ(2u, line_box_fragment.Children().size());

  // Inline boxes with box decorations generate box fragments.
  const NGPaintFragment& box_fragment = *line_box_fragment.Children()[1];
  EXPECT_EQ(NGPhysicalFragment::kFragmentBox,
            box_fragment.PhysicalFragment().Type());
  EXPECT_EQ(LayoutRect(100, 0, 30, 10), box_fragment.VisualRect());

  EXPECT_EQ(1u, box_fragment.Children().size());
  const NGPaintFragment& text_fragment = *box_fragment.Children()[0];
  EXPECT_EQ(NGPhysicalFragment::kFragmentText,
            text_fragment.PhysicalFragment().Type());

  // VisualRect of descendants of inline boxes are relative to its inline
  // formatting context, not to its parent inline box.
  EXPECT_EQ(LayoutRect(100, 0, 30, 10), text_fragment.VisualRect());
}

}  // namespace blink
