// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/editing/LayoutSelection.h"

#include "core/editing/EditingTestBase.h"
#include "core/editing/FrameSelection.h"
#include "core/layout/LayoutObject.h"
#include "platform/wtf/Assertions.h"

namespace blink {

static std::ostream& operator<<(std::ostream& ostream,
                                LayoutObject* layout_object) {
  return ostream << layout_object->GetNode()
                 << ", state:" << layout_object->GetSelectionState()
                 << (layout_object->ShouldInvalidateSelection()
                         ? ", ShoudInvalidate"
                         : ", NotInvalidate")
                 << '\n';
}

class LayoutSelectionTest : public EditingTestBase {
 public:
  // Test LayoutObject's type, SelectionState and ShouldInvalidateSelection
  // flag.
  bool TestNextLayoutObject(bool (LayoutObject::*predicate)(void) const,
                            SelectionState state) {
    if (!current_)
      current_ = GetDocument().body()->GetLayoutObject();
    else
      current_ = current_->NextInPreOrder();
    if (!current_)
      return false;
    if (!(current_->*predicate)())
      return false;
    if (current_->GetSelectionState() != state)
      return false;
    return current_->ShouldInvalidateSelection() ==
           (state != SelectionState::kNone);
  }
  LayoutObject* Current() { return current_; }
  void PrintLayoutTreeForDebug() {
    std::stringstream stream;
    for (LayoutObject* runner = GetDocument().body()->GetLayoutObject(); runner;
         runner = runner->NextInPreOrder()) {
      stream << runner;
    }
    LOG(INFO) << '\n' << stream.str();
  }

 private:
  LayoutObject* current_ = nullptr;
};

#define TEST_NEXT(predicate, state) \
  EXPECT_TRUE(TestNextLayoutObject(predicate, state)) << Current()

#define TEST_NO_NEXT_LAYOUT_OBJECT() \
  EXPECT_EQ(Current()->NextInPreOrder(), nullptr)

TEST_F(LayoutSelectionTest, TraverseLayoutObject) {
  SetBodyContent("foo<br>bar");
  Selection().SetSelection(SelectionInDOMTree::Builder()
                               .SelectAllChildren(*GetDocument().body())
                               .Build());
  Selection().CommitAppearanceIfNeeded();
  TEST_NEXT(&LayoutObject::IsLayoutBlock, SelectionState::kStartAndEnd);
  TEST_NEXT(&LayoutObject::IsText, SelectionState::kStart);
  TEST_NEXT(&LayoutObject::IsBR, SelectionState::kInside);
  TEST_NEXT(&LayoutObject::IsText, SelectionState::kEnd);
  TEST_NO_NEXT_LAYOUT_OBJECT();
}

TEST_F(LayoutSelectionTest, TraverseLayoutObjectTruncateVisibilityNone) {
  SetBodyContent(
      "<span style='visibility:hidden;'>before</span>"
      "foo"
      "<span style='visibility:hidden;'>after</span>");
  Selection().SetSelection(SelectionInDOMTree::Builder()
                               .SelectAllChildren(*GetDocument().body())
                               .Build());
  Selection().CommitAppearanceIfNeeded();
  TEST_NEXT(&LayoutObject::IsLayoutBlock, SelectionState::kStartAndEnd);
  TEST_NEXT(&LayoutObject::IsLayoutInline, SelectionState::kNone);
  TEST_NEXT(&LayoutObject::IsText, SelectionState::kNone);
  TEST_NEXT(&LayoutObject::IsText, SelectionState::kStartAndEnd);
  TEST_NEXT(&LayoutObject::IsLayoutInline, SelectionState::kNone);
  TEST_NEXT(&LayoutObject::IsText, SelectionState::kNone);
  TEST_NO_NEXT_LAYOUT_OBJECT();
}

TEST_F(LayoutSelectionTest, TraverseLayoutObjectTruncateBR) {
  SetBodyContent("<br><br>foo<br><br>");
  Selection().SetSelection(SelectionInDOMTree::Builder()
                               .SelectAllChildren(*GetDocument().body())
                               .Build());
  Selection().CommitAppearanceIfNeeded();
  TEST_NEXT(&LayoutObject::IsLayoutBlock, SelectionState::kStartAndEnd);
  TEST_NEXT(&LayoutObject::IsBR, SelectionState::kStart);
  TEST_NEXT(&LayoutObject::IsBR, SelectionState::kInside);
  TEST_NEXT(&LayoutObject::IsText, SelectionState::kInside);
  TEST_NEXT(&LayoutObject::IsBR, SelectionState::kEnd);
  TEST_NEXT(&LayoutObject::IsBR, SelectionState::kNone);
  TEST_NO_NEXT_LAYOUT_OBJECT();
}

TEST_F(LayoutSelectionTest, TraverseLayoutObjectListStyleImage) {
  SetBodyContent(
      "<style>ul {list-style-image:url(data:"
      "image/gif;base64,R0lGODlhAQABAIAAAAUEBAAAACwAAAAAAQABAAACAkQBADs=)}"
      "</style>"
      "<ul><li>foo<li>bar</ul>");
  Selection().SetSelection(SelectionInDOMTree::Builder()
                               .SelectAllChildren(*GetDocument().body())
                               .Build());
  Selection().CommitAppearanceIfNeeded();
  TEST_NEXT(&LayoutObject::IsLayoutBlock, SelectionState::kStartAndEnd);
  TEST_NEXT(&LayoutObject::IsLayoutBlockFlow, SelectionState::kStartAndEnd);
  TEST_NEXT(&LayoutObject::IsListItem, SelectionState::kStart);
  TEST_NEXT(&LayoutObject::IsListMarker, SelectionState::kNone);
  TEST_NEXT(&LayoutObject::IsText, SelectionState::kStart);
  TEST_NEXT(&LayoutObject::IsListItem, SelectionState::kEnd);
  TEST_NEXT(&LayoutObject::IsListMarker, SelectionState::kNone);
  TEST_NEXT(&LayoutObject::IsText, SelectionState::kEnd);
  TEST_NO_NEXT_LAYOUT_OBJECT();
}

}  // namespace blink
