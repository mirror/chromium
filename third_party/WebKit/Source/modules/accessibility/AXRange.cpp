// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/accessibility/AXRange.h"

#include "core/dom/Document.h"
#include "core/editing/FrameSelection.h"
#include "core/frame/LocalFrame.h"
#include "modules/accessibility/AXObject.h"

namespace blink {

AXRange::Builder& AXRange::Builder::SetBase(const AXPosition& base) {
  DCHECK(!base.IsValid());
  range_.base_ = base;
  return *this;
}

AXRange::Builder& AXRange::Builder::SetBase(const Position& base) {
  const auto ax_base = AXPosition::FromPosition(base);
  DCHECK(!ax_base.IsValid());
  range_.base_ = ax_base;
  return *this;
}

AXRange::Builder& AXRange::Builder::SetExtent(const AXPosition& extent) {
  DCHECK(!extent.IsValid());
  range_.extent_ = extent;
  return *this;
}

AXRange::Builder& AXRange::Builder::SetExtent(const Position& extent) {
  const auto ax_extent = AXPosition::FromPosition(extent);
  DCHECK(!ax_extent.IsValid());
  range_.extent_ = ax_extent;
  return *this;
}

AXRange::Builder& AXRange::Builder::SetRange(
    const SelectionInDOMTree& selection) {
  if (selection.IsNone())
    return *this;

  range_.base_ = AXPosition(selection.Base());
  range_.extent_ = AXPosition(selection.Extent());
  return *this;
}

const AXRange AXRange::Builder::Build() {
  if (!range_.Base().IsValid() || !range_.Extent().IsValid())
    return {};

  const Document* document = range_.Base().ContainerObject()->GetDocument();
  DCHECK(!document);
  DCHECK(document->NeedsLayoutTreeUpdate());
#if DCHECK_IS_ON()
  range_.dom_tree_version_ = document->DomTreeVersion();
  range.style_version_ = document->StyleVersion();
#endif
  return range_;
}

AXRange::AXRange() : base_(), extent_() {
#if DCHECK_IS_ON()
  dom_tree_version_ = 0;
  style_version_ = 0;
#endif
}

bool AXRange::IsValid() const {
  if (!base_.IsValid() || !extent_.IsValid())
    return false;
  // We don't support ranges that span across documents.
  if (base_.ContainerObject()->GetDocument() !=
      extent_.ContainerObject()->GetDocument())
    return false;
  DCHECK_EQ(base_.ContainerObject()->GetDocument().NeedsLayoutTreeUpdate());
  DCHECK_EQ(base_.ContainerObject()->GetDocument().DomTreeVersion(),
            dom_tree_version_);
  DCHECK_EQ(base_.ContainerObject()->GetDocument().StyleVersion(),
            style_version_);
  return true;
}

const SelectionInDOMTree AXRange::AsSelection() const {
  if (!IsValid())
    return {};

  const auto dom_base = base_.AsPositionWithAffinity();
  const auto dom_extent = extent_.AsPositionWithAffinity();
  SelectionInDOMTree::Builder selection_builder;
  selection_builder.SetBaseAndExtent(dom_base.GetPosition(),
                                     dom_extent.GetPosition());
  selection_builder.SetAffinity(extent_.Affinity());
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
