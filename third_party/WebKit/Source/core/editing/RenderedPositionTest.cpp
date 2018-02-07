// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/editing/RenderedPosition.h"

#include "build/build_config.h"
#include "core/editing/FrameSelection.h"
#include "core/editing/InlineBoxPosition.h"
#include "core/editing/SelectionTemplate.h"
#include "core/editing/VisiblePosition.h"
#include "core/editing/testing/EditingTestBase.h"
#include "core/frame/Settings.h"
#include "core/html/forms/HTMLInputElement.h"
#include "core/paint/compositing/CompositedSelection.h"

namespace blink {

class RenderedPositionTest : public EditingTestBase {
  void SetUp() override {
    EditingTestBase::SetUp();
    // Enable compositing.
    GetPage().GetSettings().SetAcceleratedCompositingEnabled(true);
    GetDocument().View()->SetParentVisible(true);
    GetDocument().View()->SetSelfVisible(true);
    GetDocument().View()->UpdateAllLifecyclePhases();

    LoadAhem();
  }

 protected:
  CompositedSelection ComputeCompositedSelection(Element* focus,
                                                 const Node& select) {
    DCHECK(focus);
    focus->focus();
    Selection().SetSelection(
        SelectionInDOMTree::Builder().SelectAllChildren(select).Build(),
        SetSelectionOptions::Builder().SetShouldShowHandle(true).Build());
    UpdateAllLifecyclePhases();

    return RenderedPosition::ComputeCompositedSelection(Selection());
  }
};

#if defined(OS_ANDROID)
#define MAYBE_ComputeCompositedSelection DISABLED_ComputeCompositedSelection
#else
#define MAYBE_ComputeCompositedSelection ComputeCompositedSelection
#endif
TEST_F(RenderedPositionTest, MAYBE_ComputeCompositedSelection) {
  SetBodyContent(
      "<input id=target width=20 value='test test test test test tes tes test'"
      "style='width: 100px; height: 20px;'>");
  HTMLInputElement* target =
      ToHTMLInputElement(GetDocument().getElementById("target"));
  const CompositedSelection& composited_selection =
      ComputeCompositedSelection(target, *target->InnerEditorElement());
  EXPECT_FALSE(composited_selection.start.hidden);
  EXPECT_TRUE(composited_selection.end.hidden);
}

// crbug.com/807930
TEST_F(RenderedPositionTest, ContentEditableLinebreak) {
  SetBodyContent("<div contenteditable>test<br><br></div>");
  Element* target = GetDocument().QuerySelector("div");
  const CompositedSelection& composited_selection =
      ComputeCompositedSelection(target, *target);
  EXPECT_EQ(composited_selection.start.edge_top_in_layer,
            FloatPoint(8.0f, 8.0f));
  EXPECT_EQ(composited_selection.start.edge_bottom_in_layer,
            FloatPoint(8.0f, 9.0f));
  EXPECT_EQ(composited_selection.end.edge_top_in_layer, FloatPoint(8.0f, 9.0f));
  EXPECT_EQ(composited_selection.end.edge_bottom_in_layer,
            FloatPoint(8.0f, 10.0f));
}

// crbug.com/807930
TEST_F(RenderedPositionTest, TextAreaLinebreak) {
  SetBodyContent("<textarea id=target >test\n</textarea>");
  TextControlElement* target =
      ToTextControlElement(GetDocument().getElementById("target"));
  const CompositedSelection& composited_selection =
      ComputeCompositedSelection(target, *target->InnerEditorElement());

  EXPECT_EQ(composited_selection.start.edge_top_in_layer,
            FloatPoint(11.0f, 11.0f));
  EXPECT_EQ(composited_selection.start.edge_bottom_in_layer,
            FloatPoint(11.0f, 27.0f));
  EXPECT_EQ(composited_selection.end.edge_top_in_layer,
            FloatPoint(11.0f, 27.0f));
  EXPECT_EQ(composited_selection.end.edge_bottom_in_layer,
            FloatPoint(11.0f, 43.0f));
}

}  // namespace blink
