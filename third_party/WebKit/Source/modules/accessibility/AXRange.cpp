// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/accessibility/AXRange.h"

#include "core/editing/FrameSelection.h"
#include "core/frame/LocalFrame.h"
#include "core/html/TextControlElement.h"
#include "modules/accessibility/AXObject.h"

namespace blink {

template <typename Strategy>
AXRange::AXRange(const SelectionTemplate<Strategy>& selection) : AXRange() {
  if (selection.IsNone())
    return;
  DCHECK(selection.AssertValid());
  anchor_ = AXPosition(selection.Base());
  focus_ = AXPosition(selection.Extent());
}

bool AXRange::IsValid() const {
  if (!anchor_.IsValid() || !focus_.IsValid())
    return false;
  // We don't support ranges that span across documents.
  if (AnchorObject()->GetDocument() != FocusObject()->GetDocument())
    return false;
  return true;
}

SelectionInDOMTree AXRange::AsSelectionInDOMTree() const {
  if (!IsValid())
    return {};
  Position base =
      Position::EditingPositionOf(AnchorObject()->GetNode(), *AnchorOffset());
  Position extent =
      Position::EditingPositionOf(FocusObject()->GetNode(), *FocusOffset());
  SelectionInDOMTree::Builder selection_builder;
  selection_builder.SetBaseAndExtent(base, extent);
  selection_builder.SetAffinity(FocusAffinity());
  return selection_builder.Build();
}

void AXRange::Select() {
  if (!IsValid())
    return;
  if (IsSimple() && IsTextControlElement(FocusObject()->GetNode())) {
    TextControlElement* text_control =
        ToTextControlElement(FocusObject()->GetNode());
    if (*AnchorOffset() <= *FocusOffset()) {
      text_control->SetSelectionRange(*AnchorOffset(), *FocusOffset(),
                                      kSelectionHasForwardDirection);
    } else {
      text_control->SetSelectionRange(*AnchorOffset(), *FocusOffset(),
                                      kSelectionHasBackwardDirection);
    }
    return;
  }

  const SelectionInDOMTree selection = AsSelectionInDOMTree();
  DCHECK(selection.AssertValid());
  Document* document = selection.Base().GetDocument();
  if (!document)
    return;
  LocalFrame* frame = document->GetFrame();
  if (!frame)
    return;
  FrameSelection& frame_selection = frame->Selection();
  frame_selection.SetSelection(selection);
}

}  // namespace blink
