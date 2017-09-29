/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009 Apple Inc. All rights
 * reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "core/editing/LayoutSelection.h"

#include "core/dom/Document.h"
#include "core/editing/EditingUtilities.h"
#include "core/editing/EphemeralRange.h"
#include "core/editing/FrameSelection.h"
#include "core/editing/SelectionTemplate.h"
#include "core/editing/VisiblePosition.h"
#include "core/editing/VisibleUnits.h"
#include "core/html/TextControlElement.h"
#include "core/layout/LayoutText.h"
#include "core/layout/LayoutTextFragment.h"
#include "core/layout/LayoutView.h"
#include "core/paint/PaintLayer.h"

namespace blink {

SelectionPaintRange::SelectionPaintRange(LayoutObject* start_layout_object,
                                         int start_offset,
                                         LayoutObject* end_layout_object,
                                         int end_offset)
    : start_layout_object_(start_layout_object),
      start_offset_(start_offset),
      end_layout_object_(end_layout_object),
      end_offset_(end_offset) {}

bool SelectionPaintRange::operator==(const SelectionPaintRange& other) const {
  return start_layout_object_ == other.start_layout_object_ &&
         start_offset_ == other.start_offset_ &&
         end_layout_object_ == other.end_layout_object_ &&
         end_offset_ == other.end_offset_;
}

LayoutObject* SelectionPaintRange::StartLayoutObject() const {
  DCHECK(!IsNull());
  return start_layout_object_;
}

int SelectionPaintRange::StartOffset() const {
  DCHECK(!IsNull());
  return start_offset_;
}

LayoutObject* SelectionPaintRange::EndLayoutObject() const {
  DCHECK(!IsNull());
  return end_layout_object_;
}

int SelectionPaintRange::EndOffset() const {
  DCHECK(!IsNull());
  return end_offset_;
}

SelectionPaintRange::Iterator::Iterator(const SelectionPaintRange* range) {
  if (!range || range->IsNull()) {
    current_ = nullptr;
    return;
  }
  current_ = range->StartLayoutObject();
  stop_ = range->EndLayoutObject()->NextInPreOrder();
}

LayoutObject* SelectionPaintRange::Iterator::operator*() const {
  DCHECK(current_);
  return current_;
}

SelectionPaintRange::Iterator& SelectionPaintRange::Iterator::operator++() {
  DCHECK(current_);
  current_ = current_->NextInPreOrder();
  if (current_ && current_ != stop_)
    return *this;

  current_ = nullptr;
  return *this;
}

LayoutSelection::LayoutSelection(FrameSelection& frame_selection)
    : frame_selection_(&frame_selection),
      has_pending_selection_(false),
      paint_range_(SelectionPaintRange()) {}

enum class SelectionMode {
  kNone,
  kRange,
  kBlockCursor,
};

static SelectionMode ComputeSelectionMode(
    const FrameSelection& frame_selection) {
  const SelectionInDOMTree& selection_in_dom =
      frame_selection.GetSelectionInDOMTree();
  if (selection_in_dom.IsRange())
    return SelectionMode::kRange;
  DCHECK(selection_in_dom.IsCaret());
  if (!frame_selection.ShouldShowBlockCursor())
    return SelectionMode::kNone;
  if (IsLogicalEndOfLine(CreateVisiblePosition(selection_in_dom.Base())))
    return SelectionMode::kNone;
  return SelectionMode::kBlockCursor;
}

static EphemeralRangeInFlatTree CalcSelectionInFlatTree(
    const FrameSelection& frame_selection) {
  const SelectionInDOMTree& selection_in_dom =
      frame_selection.GetSelectionInDOMTree();
  switch (ComputeSelectionMode(frame_selection)) {
    case SelectionMode::kNone:
      return {};
    case SelectionMode::kRange: {
      const PositionInFlatTree& base =
          ToPositionInFlatTree(selection_in_dom.Base());
      const PositionInFlatTree& extent =
          ToPositionInFlatTree(selection_in_dom.Extent());
      if (base.IsNull() || extent.IsNull() || base == extent ||
          !base.IsValidFor(frame_selection.GetDocument()) ||
          !extent.IsValidFor(frame_selection.GetDocument()))
        return {};
      return base <= extent ? EphemeralRangeInFlatTree(base, extent)
                            : EphemeralRangeInFlatTree(extent, base);
    }
    case SelectionMode::kBlockCursor: {
      const PositionInFlatTree& base =
          CreateVisiblePosition(ToPositionInFlatTree(selection_in_dom.Base()))
              .DeepEquivalent();
      if (base.IsNull())
        return {};
      const PositionInFlatTree end_position =
          NextPositionOf(base, PositionMoveType::kGraphemeCluster);
      if (end_position.IsNull())
        return {};
      return base <= end_position
                 ? EphemeralRangeInFlatTree(base, end_position)
                 : EphemeralRangeInFlatTree(end_position, base);
    }
  }
  NOTREACHED();
  return {};
}

// LayoutObjects each has SelectionState of kStart, kEnd, kStartAndEnd, kInside
// or kContain.
using SelectedLayoutObjects = HashSet<LayoutObject*>;
// OldSelectedLayoutObjects is previously selected LayoutObjects with
// previous SelectionState.
using OldSelectedLayoutObjects = HashMap<LayoutObject*, SelectionState>;

#ifndef NDEBUG
void PrintSelectedLayoutObjects(
    const SelectedLayoutObjects& new_selected_objects) {
  std::stringstream stream;
  stream << std::endl;
  for (LayoutObject* layout_object : new_selected_objects) {
    PrintLayoutObjectForSelection(stream, layout_object);
    stream << std::endl;
  }
  LOG(INFO) << stream.str();
}

void PrintOldSelectedLayoutObjects(
    const OldSelectedLayoutObjects& old_selected_objects) {
  std::stringstream stream;
  stream << std::endl;
  for (const auto& key_pair : old_selected_objects) {
    LayoutObject* layout_object = key_pair.key;
    SelectionState old_state = key_pair.value;
    PrintLayoutObjectForSelection(stream, layout_object);
    stream << " old: " << old_state << std::endl;
  }
  LOG(INFO) << stream.str();
}
#endif

static void InsertLayoutObjectAndAncestorBlocks(
    SelectedLayoutObjects* selected_objects,
    LayoutObject* layout_object) {
  if (!selected_objects->insert(layout_object).is_new_entry)
    return;

  for (LayoutBlock* containing_block = layout_object->ContainingBlock();
       containing_block && !containing_block->IsLayoutView();
       containing_block = containing_block->ContainingBlock()) {
    const auto& result = selected_objects->insert(containing_block);
    if (!result.is_new_entry)
      return;
  }
}

// This class represents a selection range in layout tree for marking
// SelectionState
class NewPaintRangeAndSelectedLayoutObjects {
  STACK_ALLOCATED();

 public:
  NewPaintRangeAndSelectedLayoutObjects() = default;
  NewPaintRangeAndSelectedLayoutObjects(SelectionPaintRange paint_range,
                                        SelectedLayoutObjects selected_objects)
      : paint_range_(paint_range),
        selected_objects_(std::move(selected_objects)) {}
  NewPaintRangeAndSelectedLayoutObjects(
      NewPaintRangeAndSelectedLayoutObjects&& other) {
    paint_range_ = other.paint_range_;
    selected_objects_ = std::move(other.selected_objects_);
  }

  SelectionPaintRange PaintRange() const { return paint_range_; }

  const SelectedLayoutObjects& LayoutObjects() const {
    return selected_objects_;
  }

 private:
  SelectionPaintRange paint_range_;
  SelectedLayoutObjects selected_objects_;

 private:
  DISALLOW_COPY_AND_ASSIGN(NewPaintRangeAndSelectedLayoutObjects);
};

static void InvalidateIfNeeds(LayoutObject* layout_object) {
  // LayoutBlock is not invalidated.
  if (layout_object->ShouldInvalidateSelection())
    return;
  layout_object->SetShouldInvalidateSelection();
  LayoutObject* parent = layout_object->Parent();
  if (!parent || !parent->IsSVGText())
    return;
  InvalidateIfNeeds(parent);
}

// Set ShouldInvalidateSelection flag of LayoutObjects
// comparing them in |new_range| and |old_range|.
static void SetShouldInvalidateSelection(
    const NewPaintRangeAndSelectedLayoutObjects& new_range,
    const SelectionPaintRange& old_range,
    const OldSelectedLayoutObjects& old_selected_objects) {
  // We invalidate each LayoutObject which
  // - is included in new selection range and
  // --  has valid SelectionState(!= kNone) and
  // --  is not in old invaldation map.
  for (LayoutObject* layout_object : new_range.LayoutObjects()) {
    if (old_selected_objects.Contains(layout_object))
      continue;
    if (layout_object->GetSelectionState() == SelectionState::kNone)
      continue;
    // LayoutBlock is not invalidated.
    if (layout_object->GetSelectionState() == SelectionState::kContain)
      continue;
    InvalidateIfNeeds(layout_object);
  }
  // - is included in previous selection range and
  // --  has different SelectionState to previous.
  const SelectionPaintRange& new_paint_range = new_range.PaintRange();
  for (const auto& key_value : old_selected_objects) {
    LayoutObject* const layout_object = key_value.key;
    const SelectionState old_state = key_value.value;
    const SelectionState new_state = layout_object->GetSelectionState();
    // LayoutBlock is not invalidated.
    if (old_state == SelectionState::kContain)
      continue;
    if (new_state != old_state) {
      InvalidateIfNeeds(layout_object);
      continue;
    }
    // Invalidate LayoutText which has same SelectionState
    // but offset was changed.
    if (!layout_object->IsText())
      continue;
    // Selection is in a text node and is moving on the node.
    if (new_state == SelectionState::kStartAndEnd &&
        (new_paint_range.StartOffset() != old_range.StartOffset() ||
         new_paint_range.EndOffset() != old_range.EndOffset())) {
      InvalidateIfNeeds(layout_object);
      continue;
    }
    // Selection start is moving on a same node.
    if (new_state == SelectionState::kStart &&
        new_paint_range.StartOffset() != old_range.StartOffset()) {
      InvalidateIfNeeds(layout_object);
      continue;
    }
    // Selection end is moving on a same node.
    if (new_state == SelectionState::kEnd &&
        new_paint_range.EndOffset() != old_range.EndOffset()) {
      InvalidateIfNeeds(layout_object);
      continue;
    }
  }
}

base::Optional<int> LayoutSelection::SelectionStart() const {
  DCHECK(!HasPendingSelection());
  if (paint_range_.IsNull())
    return {};
  return paint_range_.StartOffset();
}

base::Optional<int> LayoutSelection::SelectionEnd() const {
  DCHECK(!HasPendingSelection());
  if (paint_range_.IsNull())
    return {};
  return paint_range_.EndOffset();
}

static OldSelectedLayoutObjects CollectOldLayoutObjectsMarkingNone(
    const SelectionPaintRange& old_range) {
  OldSelectedLayoutObjects old_selected_objects;
  for (LayoutObject* layout_object : old_range) {
    if (old_selected_objects.Contains(layout_object))
      continue;
    old_selected_objects.insert(layout_object,
                                layout_object->GetSelectionState());
    layout_object->SetSelectionState(SelectionState::kNone);

    for (LayoutBlock* containing_block = layout_object->ContainingBlock();
         containing_block && !containing_block->IsLayoutView();
         containing_block = containing_block->ContainingBlock()) {
      if (old_selected_objects.Contains(containing_block))
        break;
      old_selected_objects.insert(containing_block,
                                  containing_block->GetSelectionState());
      containing_block->SetSelectionState(SelectionState::kNone);
    }
  }
  return old_selected_objects;
}

void LayoutSelection::ClearSelection() {
  // For querying Layer::compositingState()
  // This is correct, since destroying layout objects needs to cause eager paint
  // invalidations.
  DisableCompositingQueryAsserts disabler;

  // Just return if the selection is already empty.
  if (paint_range_.IsNull())
    return;

  for (const auto& key_value :
       CollectOldLayoutObjectsMarkingNone(paint_range_)) {
    LayoutObject* const layout_object = key_value.key;
    const SelectionState old_state = key_value.value;
    if (layout_object->GetSelectionState() == old_state)
      continue;
    layout_object->SetShouldInvalidateSelection();
  }

  // Reset selection.
  paint_range_ = SelectionPaintRange();
}

static int ComputeStartOffset(const LayoutObject& layout_object,
                              const PositionInFlatTree& position) {
  Node* const layout_node = layout_object.GetNode();
  if (!layout_node || !layout_node->IsTextNode())
    return 0;

  if (layout_node == position.AnchorNode())
    return position.OffsetInContainerNode();
  return 0;
}

static int ComputeEndOffset(const LayoutObject& layout_object,
                            const PositionInFlatTree& position) {
  Node* const layout_node = layout_object.GetNode();
  if (!layout_node || !layout_node->IsTextNode())
    return 0;

  if (layout_node == position.AnchorNode())
    return position.OffsetInContainerNode();
  return ToText(layout_node)->length();
}

static LayoutTextFragment* FirstLetterPartFor(LayoutObject* layout_object) {
  if (!layout_object->IsText())
    return nullptr;
  if (!ToLayoutText(layout_object)->IsTextFragment())
    return nullptr;
  return ToLayoutTextFragment(const_cast<LayoutObject*>(
      AssociatedLayoutObjectOf(*layout_object->GetNode(), 0)));
}

static void SetSelectionStatePropagateIfNeeds(LayoutObject* layout_object,
                                              SelectionState state) {
  // |layout_object| should be leaf.
  DCHECK(!layout_object->SlowFirstChild());
  DCHECK(state != SelectionState::kNone && state != SelectionState::kContain)
      << state;
  DCHECK(!layout_object->IsLayoutBlock()) << layout_object;

  layout_object->SetSelectionState(state);
  // This selectionstate propagation is needed for invalidation about pesudo
  // CSS ::selection element. See LayoutObject::InvalidatePaintForSelection().
  for (LayoutBlock* containing_block = layout_object->ContainingBlock();
       containing_block && !containing_block->IsLayoutView();
       containing_block = containing_block->ContainingBlock()) {
    if (containing_block->GetSelectionState() == SelectionState::kContain)
      return;
    containing_block->SetSelectionState(SelectionState::kContain);
  }
}

static void MarkSelected(SelectedLayoutObjects* invalidation_set,
                         LayoutObject* layout_object,
                         SelectionState state) {
  SetSelectionStatePropagateIfNeeds(layout_object, state);
  InsertLayoutObjectAndAncestorBlocks(invalidation_set, layout_object);
}

static void MarkSelectedInside(SelectedLayoutObjects* invalidation_set,
                               LayoutObject* layout_object) {
  MarkSelected(invalidation_set, layout_object, SelectionState::kInside);
  LayoutTextFragment* const first_letter_part =
      FirstLetterPartFor(layout_object);
  if (!first_letter_part)
    return;
  MarkSelected(invalidation_set, first_letter_part, SelectionState::kInside);
}

static NewPaintRangeAndSelectedLayoutObjects MarkStartAndEndInOneNode(
    SelectedLayoutObjects invalidation_set,
    LayoutObject* layout_object,
    int start_offset,
    int end_offset) {
  DCHECK_GE(start_offset, 0);
  DCHECK_GE(end_offset, 0);
  LayoutTextFragment* const first_letter_part =
      FirstLetterPartFor(layout_object);
  if (!first_letter_part) {
    // Case 0: selection doesn't start/end in ::first-letter node
    if (layout_object->IsText()) {
      DCHECK_LE(start_offset, end_offset);
      if (start_offset == end_offset)
        return {};
    }
    MarkSelected(&invalidation_set, layout_object,
                 SelectionState::kStartAndEnd);
    return {{layout_object, start_offset, layout_object, end_offset},
            std::move(invalidation_set)};
  }
  DCHECK_LE(start_offset, end_offset);
  if (start_offset == end_offset)
    return {};
  LayoutTextFragment* const remaining_part =
      ToLayoutTextFragment(layout_object);
  if (static_cast<unsigned>(start_offset) >= remaining_part->Start()) {
    // Case 1: The selection starts and ends in remaining part.
    DCHECK_GT(static_cast<unsigned>(end_offset), remaining_part->Start());
    MarkSelected(&invalidation_set, remaining_part,
                 SelectionState::kStartAndEnd);
    return {{remaining_part,
             static_cast<int>(start_offset - remaining_part->Start()),
             remaining_part,
             static_cast<int>(end_offset - remaining_part->Start())},
            std::move(invalidation_set)};
  }
  if (static_cast<unsigned>(end_offset) <= remaining_part->Start()) {
    // Case 2: The selection starts and ends in first letter part.
    MarkSelected(&invalidation_set, first_letter_part,
                 SelectionState::kStartAndEnd);
    return {{first_letter_part, start_offset, first_letter_part, end_offset},
            std::move(invalidation_set)};
  }

  // Case 3: The selection starts in first-letter part and ends in remaining
  // part.
  DCHECK_GT(static_cast<unsigned>(end_offset), remaining_part->Start());
  MarkSelected(&invalidation_set, first_letter_part, SelectionState::kStart);
  MarkSelected(&invalidation_set, remaining_part, SelectionState::kEnd);
  return {{first_letter_part, start_offset, remaining_part,
           static_cast<int>(end_offset - remaining_part->Start())},
          std::move(invalidation_set)};
}

static NewPaintRangeAndSelectedLayoutObjects MarkStartAndEndInTwoNodes(
    SelectedLayoutObjects invalidation_set,
    LayoutObject* start_layout_object,
    int start_offset,
    LayoutObject* end_layout_object,
    int end_offset) {
  DCHECK_NE(start_layout_object, end_layout_object);
  DCHECK_GE(start_offset, 0);
  DCHECK_GE(end_offset, 0);
  LayoutTextFragment* const start_first_letter_part =
      FirstLetterPartFor(start_layout_object);
  LayoutTextFragment* const end_first_letter_part =
      FirstLetterPartFor(end_layout_object);
  if (!start_first_letter_part && !end_first_letter_part) {
    // Case 0: Both start and end don't relate to first-letter.
    MarkSelected(&invalidation_set, start_layout_object,
                 SelectionState::kStart);
    MarkSelected(&invalidation_set, end_layout_object, SelectionState::kEnd);
    return {{start_layout_object, start_offset, end_layout_object, end_offset},
            std::move(invalidation_set)};
  }
  if (!start_first_letter_part) {
    LayoutTextFragment* const end_remaining_part =
        ToLayoutTextFragment(end_layout_object);
    if (static_cast<unsigned>(end_offset) <= end_remaining_part->Start()) {
      // Case 1: The selection ends in first-letter part
      MarkSelected(&invalidation_set, start_layout_object,
                   SelectionState::kStart);
      MarkSelected(&invalidation_set, end_first_letter_part,
                   SelectionState::kEnd);
      return {{start_layout_object, start_offset, end_first_letter_part,
               end_offset},
              std::move(invalidation_set)};
    }
    // Case 2: The selection ends in remaining part
    DCHECK_GT(static_cast<unsigned>(end_offset), end_remaining_part->Start());
    MarkSelected(&invalidation_set, start_layout_object,
                 SelectionState::kStart);
    MarkSelected(&invalidation_set, end_first_letter_part,
                 SelectionState::kInside);
    MarkSelected(&invalidation_set, end_remaining_part, SelectionState::kEnd);
    return {{start_layout_object, start_offset, end_remaining_part,
             static_cast<int>(end_offset - end_remaining_part->Start())},
            std::move(invalidation_set)};
  }
  if (!end_first_letter_part) {
    LayoutTextFragment* const start_remaining_part =
        ToLayoutTextFragment(start_layout_object);
    if (static_cast<unsigned>(start_offset) < start_remaining_part->Start()) {
      // Case 3: The selection starts in first-letter part.
      MarkSelected(&invalidation_set, start_first_letter_part,
                   SelectionState::kStart);
      MarkSelected(&invalidation_set, start_remaining_part,
                   SelectionState::kInside);
      MarkSelected(&invalidation_set, end_layout_object, SelectionState::kEnd);
      return {{start_first_letter_part, start_offset, end_layout_object,
               end_offset},
              std::move(invalidation_set)};
    }
    // Case 4: The selection starts in remaining part.
    MarkSelected(&invalidation_set, start_remaining_part,
                 SelectionState::kStart);
    MarkSelected(&invalidation_set, end_layout_object, SelectionState::kEnd);
    return {{start_remaining_part,
             static_cast<int>(start_offset - start_remaining_part->Start()),
             end_layout_object, end_offset},
            std::move(invalidation_set)};
  }
  LayoutTextFragment* const start_remaining_part =
      ToLayoutTextFragment(start_layout_object);
  LayoutTextFragment* const end_remaining_part =
      ToLayoutTextFragment(end_layout_object);
  if (static_cast<unsigned>(start_offset) < start_remaining_part->Start() &&
      static_cast<unsigned>(end_offset) <= end_remaining_part->Start()) {
    // Case 5: The selection starts and end in first-letter part.
    MarkSelected(&invalidation_set, start_first_letter_part,
                 SelectionState::kStart);
    MarkSelected(&invalidation_set, start_remaining_part,
                 SelectionState::kInside);
    MarkSelected(&invalidation_set, end_first_letter_part,
                 SelectionState::kEnd);
    return {{start_first_letter_part, start_offset, end_first_letter_part,
             end_offset},
            std::move(invalidation_set)};
  }
  if (static_cast<unsigned>(start_offset) < start_remaining_part->Start()) {
    // Case 6: The selection starts in first-letter part and ends in remaining
    // part.
    DCHECK_GT(static_cast<unsigned>(end_offset), end_remaining_part->Start());
    MarkSelected(&invalidation_set, start_first_letter_part,
                 SelectionState::kStart);
    MarkSelected(&invalidation_set, start_remaining_part,
                 SelectionState::kInside);
    MarkSelected(&invalidation_set, end_first_letter_part,
                 SelectionState::kInside);
    MarkSelected(&invalidation_set, end_remaining_part, SelectionState::kEnd);
    return {{start_first_letter_part, start_offset, end_remaining_part,
             static_cast<int>(end_offset - end_remaining_part->Start())},
            std::move(invalidation_set)};
  }
  if (static_cast<unsigned>(end_offset) <= end_remaining_part->Start()) {
    // Case 7: The selection starts in remaining part and ends in first letter
    // part.
    DCHECK_GE(static_cast<unsigned>(start_offset),
              start_remaining_part->Start());
    MarkSelected(&invalidation_set, start_remaining_part,
                 SelectionState::kStart);
    MarkSelected(&invalidation_set, end_first_letter_part,
                 SelectionState::kEnd);
    return {{start_remaining_part,
             static_cast<int>(start_offset - start_remaining_part->Start()),
             end_first_letter_part, end_offset},
            std::move(invalidation_set)};
  }
  // Case 8: The selection starts in remaining part and ends in remaining part.
  DCHECK_GE(static_cast<unsigned>(start_offset), start_remaining_part->Start());
  DCHECK_GT(static_cast<unsigned>(end_offset), end_remaining_part->Start());
  MarkSelected(&invalidation_set, start_remaining_part, SelectionState::kStart);
  MarkSelected(&invalidation_set, end_first_letter_part,
               SelectionState::kInside);
  MarkSelected(&invalidation_set, end_remaining_part, SelectionState::kEnd);
  return {{start_remaining_part,
           static_cast<int>(start_offset - start_remaining_part->Start()),
           end_remaining_part,
           static_cast<int>(end_offset - end_remaining_part->Start())},
          std::move(invalidation_set)};
}

static NewPaintRangeAndSelectedLayoutObjects
CalcSelectionRangeAndSetSelectionState(const FrameSelection& frame_selection) {
  const SelectionInDOMTree& selection_in_dom =
      frame_selection.GetSelectionInDOMTree();
  if (selection_in_dom.IsNone())
    return {};

  const EphemeralRangeInFlatTree& selection =
      CalcSelectionInFlatTree(frame_selection);
  if (selection.IsCollapsed() || frame_selection.IsHidden())
    return {};

  // Find first/last visible LayoutObject while
  // marking SelectionState and collecting invalidation candidate LayoutObjects.
  LayoutObject* start_layout_object = nullptr;
  LayoutObject* end_layout_object = nullptr;
  SelectedLayoutObjects selected_objects;
  for (const Node& node : selection.Nodes()) {
    LayoutObject* const layout_object = node.GetLayoutObject();
    if (!layout_object || !layout_object->CanBeSelectionLeaf() ||
        layout_object->Style()->Visibility() != EVisibility::kVisible)
      continue;

    if (!start_layout_object) {
      DCHECK(!end_layout_object);
      start_layout_object = end_layout_object = layout_object;
      continue;
    }

    // In this loop, |end_layout_object| is pointing current last candidate
    // LayoutObject and if it is not start and we find next, we mark the
    // current one as kInside.
    if (end_layout_object != start_layout_object)
      MarkSelectedInside(&selected_objects, end_layout_object);
    end_layout_object = layout_object;
  }

  // No valid LayOutObject found.
  if (!start_layout_object) {
    DCHECK(!end_layout_object);
    return {};
  }

  // Compute offset if start/end is text.
  // TODO(yoichio): Use Option<int> and return it in SelectionState()/End().
  const int start_offset = ComputeStartOffset(
      *start_layout_object, selection.StartPosition().ToOffsetInAnchor());
  const int end_offset = ComputeEndOffset(
      *end_layout_object, selection.EndPosition().ToOffsetInAnchor());

  if (start_layout_object == end_layout_object) {
    return MarkStartAndEndInOneNode(std::move(selected_objects),
                                    start_layout_object, start_offset,
                                    end_offset);
  }
  return MarkStartAndEndInTwoNodes(std::move(selected_objects),
                                   start_layout_object, start_offset,
                                   end_layout_object, end_offset);
}

void LayoutSelection::SetHasPendingSelection() {
  has_pending_selection_ = true;
}

void LayoutSelection::Commit() {
  if (!HasPendingSelection())
    return;
  has_pending_selection_ = false;

  DCHECK(!frame_selection_->GetDocument().NeedsLayoutTreeUpdate());
  DCHECK_GE(frame_selection_->GetDocument().Lifecycle().GetState(),
            DocumentLifecycle::kLayoutClean);
  DocumentLifecycle::DisallowTransitionScope disallow_transition(
      frame_selection_->GetDocument().Lifecycle());

  const OldSelectedLayoutObjects& old_selected_objects =
      CollectOldLayoutObjectsMarkingNone(paint_range_);
  const NewPaintRangeAndSelectedLayoutObjects& new_range =
      CalcSelectionRangeAndSetSelectionState(*frame_selection_);
  DCHECK(frame_selection_->GetDocument().GetLayoutView()->GetFrameView());
  SetShouldInvalidateSelection(new_range, paint_range_, old_selected_objects);

  paint_range_ = new_range.PaintRange();
  if (paint_range_.IsNull())
    return;
  if (paint_range_.StartLayoutObject() == paint_range_.EndLayoutObject()) {
    DCHECK_EQ(paint_range_.StartLayoutObject()->GetSelectionState(),
              SelectionState::kStartAndEnd);
    return;
  }
  DCHECK_EQ(paint_range_.StartLayoutObject()->GetSelectionState(),
            SelectionState::kStart);
  DCHECK_EQ(paint_range_.EndLayoutObject()->GetSelectionState(),
            SelectionState::kEnd);
}

void LayoutSelection::OnDocumentShutdown() {
  has_pending_selection_ = false;
  paint_range_ = SelectionPaintRange();
}

static LayoutRect SelectionRectForLayoutObject(const LayoutObject* object) {
  if (!object->IsRooted())
    return LayoutRect();

  if (!object->CanUpdateSelectionOnRootLineBoxes())
    return LayoutRect();

  return object->SelectionRectInViewCoordinates();
}

IntRect LayoutSelection::SelectionBounds() {
  Commit();
  if (paint_range_.IsNull())
    return IntRect();

  SelectedLayoutObjects selected_objects;
  for (LayoutObject* runner : paint_range_)
    InsertLayoutObjectAndAncestorBlocks(&selected_objects, runner);

  // Create a single bounding box rect that encloses the whole selection.
  LayoutRect selected_rect;
  for (auto layout_object : selected_objects)
    selected_rect.Unite(SelectionRectForLayoutObject(layout_object));

  return PixelSnappedIntRect(selected_rect);
}

void LayoutSelection::InvalidatePaintForSelection() {
  if (paint_range_.IsNull())
    return;

  for (LayoutObject* runner : paint_range_) {
    if (runner->GetSelectionState() == SelectionState::kNone)
      continue;

    runner->SetShouldInvalidateSelection();
  }
}

DEFINE_TRACE(LayoutSelection) {
  visitor->Trace(frame_selection_);
}

void PrintLayoutObjectForSelection(std::ostream& ostream,
                                   LayoutObject* layout_object) {
  if (!layout_object) {
    ostream << "<null>";
    return;
  }
  ostream << (void*)layout_object << ' ' << layout_object->GetNode()
          << ", state:" << layout_object->GetSelectionState()
          << (layout_object->ShouldInvalidateSelection() ? ", ShouldInvalidate"
                                                         : ", NotInvalidate");
}
#ifndef NDEBUG
void ShowLayoutObjectForSelection(LayoutObject* layout_object) {
  std::stringstream stream;
  PrintLayoutObjectForSelection(stream, layout_object);
  LOG(INFO) << '\n' << stream.str();
}
#endif

}  // namespace blink
