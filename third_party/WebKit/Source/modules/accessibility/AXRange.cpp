// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/accessibility/AXRange.h"

#include "core/dom/Document.h"
#include "core/editing/FrameSelection.h"
#include "core/frame/LocalFrame.h"
#include "modules/accessibility/AXObject.h"

namespace blink {

AXRange::AXRange() : anchor_(), focus_() {
#if DCHECK_IS_ON()
  dom_tree_version_ = 0;
#endif
}

AXRange::AXRange(AXObject* anchor_object,
                 int anchor_offset,
                 TextAffinity anchor_affinity,
                 AXObject* focus_object,
                 int focus_offset,
                 TextAffinity focus_affinity)
    : AXRange(
          anchor_(AXPosition(anchor_object, anchor_offset, anchor_affinity)),
          focus_(AXPosition(focus_object, focus_offset, focus_affinity))) {}

AXRange::AXRange(const AXPosition& anchor, const AXPosition& focus)
    : anchor_(anchor), focus_(focus) {
  if (focus_.IsValid()) {
    const Document* document = FocusObject()->GetDocument();
    DCHECK(document);
    DCHECK(!document->NeedsLayoutTreeUpdate());
#if DCHECK_IS_ON()
    dom_tree_version_ = document->DomTreeVersion();
#endif
  }
}

AXRange::AXRange(const SelectionInDOMTree& selection) : AXRange() {
  if (selection.IsNone())
    return;

  const Document* document = selection.GetDocument();
  // Non-none selections should always have a document.
  DCHECK(document);
  DCHECK(!document->NeedsLayoutTreeUpdate());
#if DCHECK_IS_ON()
  dom_tree_version_ = document->DomTreeVersion();
#endif

  anchor_ = AXPosition(selection.Base());
  focus_ = AXPosition(selection.Extent());
}

bool AXRange::IsValid() const {
  if (!anchor_.IsValid() || !focus_.IsValid())
    return false;
  // We don't support ranges that span across documents.
  if (AnchorObject()->GetDocument() != FocusObject()->GetDocument())
    return false;
  DCHECK_EQ(FocusObject()->GetDocument().DomTreeVersion(), dom_tree_version_);
  return true;
}

SelectionInDOMTree AXRange::AsSelection() const {
  if (!IsValid())
    return {};

  const auto base = anchor_.AsPositionWithAffinity();
  const auto extent = focus_.AsPositionWithAffinity();
  SelectionInDOMTree::Builder selection_builder;
  selection_builder.SetBaseAndExtent(base.GetPosition(), extent.GetPosition());
  selection_builder.SetAffinity(FocusAffinity());
  return selection_builder.Build();
}

void AXRange::Select() {
  if (!IsValid())
    return;

  const SelectionInDOMTree selection = AsSelection();
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
