// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/editing/commands/IndentOutdentCommand.h"

#include "core/editing/FrameSelection.h"
#include "core/editing/SelectionTemplate.h"
#include "core/editing/testing/EditingTestBase.h"

namespace blink {

class IndentOutdentCommandTest : public EditingTestBase {};

// http://crbug.com/426641. Do not crash.
TEST_F(IndentOutdentCommandTest, DoNotCrashOnLayerdCE) {
  SetBodyContent(
      "<div contenteditable>"
      "<span contenteditable=false>"
      "<span contenteditable><span>foo</span><ol>bar</ol></span>"
      "<ol>bar</ol>"
      "</span>"
      "<ol>bar</ol>"
      "</div>");
  GetDocument().setDesignMode("on");
  UpdateAllLifecyclePhases();
  Selection().SelectAll();
  IndentOutdentCommand* command = IndentOutdentCommand::Create(
      GetDocument(), IndentOutdentCommand::kIndent);
  // This call should not crash.
  command->Apply();
}

}  // namespace blink
