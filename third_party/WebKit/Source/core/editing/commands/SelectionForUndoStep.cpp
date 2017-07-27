// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/editing/commands/SelectionForUndoStep.h"

#include "core/editing/EditingUtilities.h"
#include "core/editing/TextAffinity.h"

namespace blink {

SelectionForUndoStep SelectionForUndoStep::From(
    const SelectionInDOMTree& selection) {
  SelectionForUndoStep result;
  result.base_ = selection.Base();
  result.extent_ = selection.Extent();
  result.affinity_ = selection.Affinity();
  result.is_directional_ = selection.IsDirectional();
  result.is_base_first_ =
      result.base_.IsNull() || result.base_ <= result.extent_;
  return result;
}

SelectionForUndoStep::SelectionForUndoStep(const SelectionForUndoStep& other)
    : base_(other.base_),
      extent_(other.extent_),
      affinity_(other.affinity_),
      is_base_first_(other.is_base_first_),
      is_directional_(other.is_directional_) {}

SelectionForUndoStep::SelectionForUndoStep() = default;

SelectionForUndoStep& SelectionForUndoStep::operator=(
    const SelectionForUndoStep& other) {
  base_ = other.base_;
  extent_ = other.extent_;
  affinity_ = other.affinity_;
  is_base_first_ = other.is_base_first_;
  is_directional_ = other.is_directional_;
  return *this;
}

SelectionInDOMTree SelectionForUndoStep::AsSelection() const {
  SelectionInDOMTree::Builder builder;
  if (base_.IsNotNull())
    builder.SetBaseAndExtent(base_, extent_);
  return builder.SetAffinity(affinity_)
      .SetIsDirectional(is_directional_)
      .Build();
}

Position SelectionForUndoStep::Start() const {
  return is_base_first_ ? base_ : extent_;
}

Position SelectionForUndoStep::End() const {
  return is_base_first_ ? extent_ : base_;
}

bool SelectionForUndoStep::IsCaret() const {
  return base_.IsNotNull() && base_ == extent_;
}

bool SelectionForUndoStep::IsNone() const {
  return base_.IsNull();
}

bool SelectionForUndoStep::IsRange() const {
  return base_ != extent_;
}

bool SelectionForUndoStep::IsValidFor(const Document& document) const {
  if (base_.IsNull())
    return true;
  if (base_.IsOrphan() || extent_.IsOrphan())
    return false;
  return base_.GetDocument() == document && extent_.GetDocument() == document;
}

VisibleSelection CreateVisibleSelection(
    const SelectionForUndoStep& selection_in_undo_step) {
  return CreateVisibleSelection(selection_in_undo_step.AsSelection());
}

DEFINE_TRACE(SelectionForUndoStep) {
  visitor->Trace(base_);
  visitor->Trace(extent_);
}

}  // namespace blink
