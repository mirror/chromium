// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/editing/commands/InsertListCommand.h"

#include "core/dom/ParentNode.h"
#include "core/dom/Text.h"
#include "core/editing/FrameSelection.h"
#include "core/editing/SelectionTemplate.h"
#include "core/editing/testing/EditingTestBase.h"
#include "core/frame/Settings.h"

namespace blink {

class InsertListCommandTest : public EditingTestBase {};

TEST_F(InsertListCommandTest, ShouldCleanlyRemoveSpuriousTextNode) {
  // Needs to be editable to use InsertListCommand.
  GetDocument().setDesignMode("on");
  // Set up the condition:
  // * Selection is a range, to go down into
  //   InsertListCommand::listifyParagraph.
  // * The selection needs to have a sibling list to go down into
  //   InsertListCommand::mergeWithNeighboringLists.
  // * Should be the same type (ordered list) to go into
  //   CompositeEditCommand::mergeIdenticalElements.
  // * Should have no actual children to fail the listChildNode check
  //   in InsertListCommand::doApplyForSingleParagraph.
  // * There needs to be an extra text node to trigger its removal in
  //   CompositeEditCommand::mergeIdenticalElements.
  // The removeNode is what updates document lifecycle to VisualUpdatePending
  // and makes FrameView::needsLayout return true.
  SetBodyContent("\nd\n<ol>");
  Text* empty_text = GetDocument().createTextNode("");
  GetDocument().body()->InsertBefore(empty_text,
                                     GetDocument().body()->firstChild());
  UpdateAllLifecyclePhases();
  GetDocument().GetFrame()->Selection().SetSelection(
      SelectionInDOMTree::Builder()
          .Collapse(Position(GetDocument().body(), 0))
          .Extend(Position(GetDocument().body(), 2))
          .Build());

  InsertListCommand* command =
      InsertListCommand::Create(GetDocument(), InsertListCommand::kOrderedList);
  // This should not DCHECK.
  EXPECT_TRUE(command->Apply())
      << "The insert ordered list command should have succeeded";
  EXPECT_EQ("<ol><li>d</li></ol>", GetDocument().body()->InnerHTMLAsString());
}

// Refer https://crbug.com/794356
TEST_F(InsertListCommandTest, UnlistifyParagraphCrashOnRemoveStyle) {
  GetDocument().setDesignMode("on");
  GetDocument().GetSettings()->SetScriptEnabled(true);
  SetBodyContent(
      "<style>"
      "  textArea {"
      "  float:left; visibility:visible; "
      "  }"
      "</style>"
      "<dl>"
      "<textarea></textarea>"
      "</dl>");
  Element* script = GetDocument().createElement("script");
  script->SetInnerHTMLFromString(
      "function handler() {"
      "  document.execCommand('selectAll');"
      "  document.execCommand('insertUnorderedList');"
      "}"
      "document.addEventListener('DOMNodeRemoved', handler);"
      "document.querySelector('style').remove();");
  GetDocument().body()->AppendChild(script);
  GetDocument().View()->UpdateAllLifecyclePhases();
  EXPECT_EQ(
      "<dl><ul>"
      "<textarea></textarea>"
      "</ul></dl>"
      "<script>"
      "function handler() {"
      "  document.execCommand('selectAll');"
      "  document.execCommand('insertUnorderedList');"
      "}"
      "document.addEventListener('DOMNodeRemoved', handler);"
      "document.querySelector('style').remove();"
      "</script>",
      GetDocument().body()->InnerHTMLAsString());
}
}
