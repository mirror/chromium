// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/editing/commands/SelectionInUndoStep.h"

#include "core/editing/EditingUtilities.h"
#include "core/editing/TextAffinity.h"

namespace blink {

namespace {

VisibleSelection CreateVisibleSelectionDeprecated(
    const SelectionInUndoStep& selection) {
  if (selection.Base().IsNull())
    return VisibleSelection();
  // TODO(editing-dev): Hoist updateStyleAndLayoutIgnorePendingStylesheets
  // to caller. See http://crbug.com/590369 for more details.
  selection.Base()
      .GetDocument()
      ->UpdateStyleAndLayoutIgnorePendingStylesheets();
  return CreateVisibleSelection(selection);
}

}  // namespace

SelectionInUndoStep SelectionInUndoStep::From(
    const SelectionInDOMTree& selection) {
  SelectionInUndoStep result;
  result.base_ = selection.Base();
  result.extent_ = selection.Extent();
  result.affinity_ = selection.Affinity();
  result.is_directional_ = selection.IsDirectional();
  return result;
}

SelectionInUndoStep::SelectionInUndoStep(const SelectionInUndoStep& other)
    : base_(other.base_),
      extent_(other.extent_),
      affinity_(other.affinity_),
      did_compute_is_base_first_(other.did_compute_is_base_first_),
      is_base_first_(other.is_base_first_),
      is_directional_(other.is_directional_) {}

SelectionInUndoStep::SelectionInUndoStep() = default;

SelectionInUndoStep& SelectionInUndoStep::operator=(
    const SelectionInUndoStep& other) {
  base_ = other.base_;
  extent_ = other.extent_;
  affinity_ = other.affinity_;
  did_compute_is_base_first_ = other.did_compute_is_base_first_;
  is_base_first_ = other.is_base_first_;
  is_directional_ = other.is_directional_;
  return *this;
}

SelectionInDOMTree SelectionInUndoStep::AsSelection() const {
  SelectionInDOMTree::Builder builder;
  if (base_.IsNotNull())
    builder.SetBaseAndExtent(base_, extent_);
  return builder.SetAffinity(affinity_)
      .SetIsDirectional(is_directional_)
      .Build();
}

bool SelectionInUndoStep::IsBaseFirst() const {
  EnsureStartAndEnd();
  return is_base_first_;
}

Position SelectionInUndoStep::Start() const {
  EnsureStartAndEnd();
  return is_base_first_ ? base_ : extent_;
}

Position SelectionInUndoStep::End() const {
  EnsureStartAndEnd();
  return is_base_first_ ? extent_ : base_;
}

bool SelectionInUndoStep::IsCaret() const {
  return base_.IsNotNull() && base_ == extent_;
}

bool SelectionInUndoStep::IsNone() const {
  return base_.IsNull();
}

bool SelectionInUndoStep::IsRange() const {
  return base_ != extent_;
}

bool SelectionInUndoStep::IsNonOrphanedCaretOrRange() const {
  return CreateVisibleSelectionDeprecated(*this).IsNonOrphanedCaretOrRange();
}

bool SelectionInUndoStep::IsValidFor(const Document& document) const {
  if (base_.IsNull())
    return true;
  if (base_.IsOrphan() || extent_.IsOrphan())
    return false;
  return base_.GetDocument() == document && extent_.GetDocument() == document;
}

Element* SelectionInUndoStep::RootEditableElement() const {
  return RootEditableElementOf(base_);
}

VisiblePosition SelectionInUndoStep::VisibleStart() const {
  return CreateVisibleSelectionDeprecated(*this).VisibleStart();
}

VisiblePosition SelectionInUndoStep::VisibleEnd() const {
  return CreateVisibleSelectionDeprecated(*this).VisibleEnd();
}

void SelectionInUndoStep::EnsureStartAndEnd() const {
  if (did_compute_is_base_first_)
    return;
  did_compute_is_base_first_ = true;
  is_base_first_ = base_ <= extent_;
}

VisibleSelection CreateVisibleSelection(
    const SelectionInUndoStep& selection_in_undo_step) {
  return CreateVisibleSelection(selection_in_undo_step.AsSelection());
}

DEFINE_TRACE(SelectionInUndoStep) {
  visitor->Trace(base_);
  visitor->Trace(extent_);
}

}  // namespace blink
