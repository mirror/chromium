// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/editing/commands/SelectionInUndoStep.h"

#include "core/editing/TextAffinity.h"

namespace blink {

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
      is_base_first_(other.is_base_first_),
      is_directional_(other.is_directional_),
      should_compute_is_base_first_(other.should_compute_is_base_first_) {}

SelectionInUndoStep::SelectionInUndoStep() = default;

SelectionInUndoStep& SelectionInUndoStep::operator=(
    const SelectionInUndoStep& other) {
  base_ = other.base_;
  extent_ = other.extent_;
  affinity_ = other.affinity_;
  is_base_first_ = other.is_base_first_;
  is_directional_ = other.is_directional_;
  should_compute_is_base_first_ = other.should_compute_is_base_first_;
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
  return CreateVisibleSelection(*this).IsCaret();
}

bool SelectionInUndoStep::IsNone() const {
  return CreateVisibleSelection(*this).IsNone();
}

bool SelectionInUndoStep::IsRange() const {
  return CreateVisibleSelection(*this).IsRange();
}

bool SelectionInUndoStep::IsNonOrphanedCaretOrRange() const {
  return CreateVisibleSelection(*this).IsNonOrphanedCaretOrRange();
}

bool SelectionInUndoStep::IsNonOrphanedRange() const {
  return CreateVisibleSelection(*this).IsNonOrphanedRange();
}

Element* SelectionInUndoStep::RootEditableElement() const {
  return CreateVisibleSelection(*this).RootEditableElement();
}

VisiblePosition SelectionInUndoStep::VisibleStart() const {
  return CreateVisibleSelection(*this).VisibleStart();
}

VisiblePosition SelectionInUndoStep::VisibleEnd() const {
  return CreateVisibleSelection(*this).VisibleEnd();
}

void SelectionInUndoStep::EnsureStartAndEnd() const {
  if (!should_compute_is_base_first_)
    return;
  should_compute_is_base_first_ = false;
  is_base_first_ = base_ < extent_;
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
