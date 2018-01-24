// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/editing/RenderedPosition.h"

#include "build/build_config.h"
#include "core/editing/FrameSelection.h"
#include "core/editing/SelectionTemplate.h"
#include "core/frame/FrameTestHelpers.h"
#include "core/frame/WebLocalFrameImpl.h"
#include "core/html/forms/HTMLInputElement.h"
#include "core/paint/compositing/CompositedSelection.h"

namespace blink {
class RenderedPositionTest : public ::testing::Test {
 protected:
  RenderedPositionTest() {
    web_view_helper_.Initialize(nullptr, &web_view_client_);
    web_view_helper_.Resize(WebSize(640, 480));
  }

  LocalFrame& GetFrame() const {
    LocalFrame* frame = web_view_helper_.LocalMainFrame()->GetFrame();
    DCHECK(frame);
    return *frame;
  }

  Document& GetDocument() const {
    Document* document = GetFrame().GetDocument();
    DCHECK(document);
    return *document;
  }

  FrameSelection& Selection() { return GetFrame().Selection(); }

  void UpdateAllLifecyclePhases() const {
    GetDocument().View()->UpdateAllLifecyclePhases();
  }

  FrameTestHelpers::TestWebViewClient web_view_client_;
  FrameTestHelpers::WebViewHelper web_view_helper_;
};

#if defined(OS_ANDROID)
#define MAYBE_ComputeCompositedSelection DISABLED_ComputeCompositedSelection
#else
#define MAYBE_ComputeCompositedSelection ComputeCompositedSelection
#endif
TEST_F(RenderedPositionTest, MAYBE_ComputeCompositedSelection) {
  GetDocument().body()->SetInnerHTMLFromString(
      "<input id=target width=20 value='test test test test test tes tes test'"
      "style='width: 100px; height: 20px;'>",
      ASSERT_NO_EXCEPTION);
  UpdateAllLifecyclePhases();

  HTMLInputElement* target =
      ToHTMLInputElement(GetDocument().getElementById("target"));
  DCHECK(target);
  target->focus();
  Selection().SetSelection(
      SelectionInDOMTree::Builder()
          .SelectAllChildren(*target->InnerEditorElement())
          .Build(),
      SetSelectionOptions::Builder().SetShouldShowHandle(true).Build());
  UpdateAllLifecyclePhases();

  const CompositedSelection& composited_selection =
      RenderedPosition::ComputeCompositedSelection(Selection());
  EXPECT_FALSE(composited_selection.start.hidden);
  EXPECT_TRUE(composited_selection.end.hidden);
}

}  // namespace blink
