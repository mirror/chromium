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
#include "core/editing/FrameSelection.h"
#include "core/editing/VisiblePosition.h"
#include "core/editing/VisibleUnits.h"
#include "core/html/TextControlElement.h"
#include "core/layout/LayoutBlock.h"
#include "core/layout/LayoutObject.h"
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
      end_offset_(end_offset) {
  DCHECK(start_layout_object_);
  DCHECK(end_layout_object_);
  if (start_layout_object_ != end_layout_object_)
    return;
  DCHECK_LT(start_offset_, end_offset_);
}

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
  if (!range) {
    current_ = nullptr;
    return;
  }
  current_ = range->StartLayoutObject();
  included_end_ = range->EndLayoutObject();
  stop_ = range->EndLayoutObject()->ChildAt(range->EndOffset());
  if (stop_)
    return;
  stop_ = range->EndLayoutObject()->NextInPreOrderAfterChildren();
}

LayoutObject* SelectionPaintRange::Iterator::operator*() const {
  DCHECK(current_);
  return current_;
}

SelectionPaintRange::Iterator& SelectionPaintRange::Iterator::operator++() {
  DCHECK(current_);
  for (current_ = current_->NextInPreOrder(); current_ && current_ != stop_;
       current_ = current_->NextInPreOrder()) {
    if (current_ == included_end_ || current_->CanBeSelectionLeaf())
      return *this;
  }

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

Node* ComputeNodeAfterPosition(const PositionInFlatTree& position) {
  if (!position.AnchorNode())
    return 0;

  Node& const anchor_node = *position.AnchorNode();
  switch (position.AnchorType()) {
    case PositionAnchorType::kBeforeChildren: {
      if (Node* first_child = FlatTreeTraversal::FirstChild(anchor_node))
        return first_child;
      FlatTreeTraversal::NextSkippingChildren(anchor_node);
    }
    case PositionAnchorType::kAfterChildren:
      return FlatTreeTraversal::NextSkippingChildren(anchor_node);
    case PositionAnchorType::kOffsetInAnchor: {
      if (anchor_node.IsCharacterDataNode())
        return FlatTreeTraversal::Next(anchor_node);
      if (Node* child_at = FlatTreeTraversal::ChildAt(
              anchor_node, position.OffsetInContainerNode()))
        return child_at;
      return FlatTreeTraversal::Next(anchor_node);
    }
    case PositionAnchorType::kBeforeAnchor:
      return &anchor_node;
    case PositionAnchorType::kAfterAnchor:
      return FlatTreeTraversal::NextSkippingChildren(anchor_node);
  }
  NOTREACHED();
  return 0;
}

static PositionInFlatTree FirstLayoutPosition(const PositionInFlatTree& start) {
  if (start.AnchorNode()->IsTextNode() &&
      start.AnchorNode()->GetLayoutObject()) {
    return start;
  }

  for (Node* runner = ComputeNodeAfterPosition(start); runner;
       runner = FlatTreeTraversal::Next(*runner)) {
    if (runner->GetLayoutObject())
      return {runner, PositionAnchorType::kBeforeAnchor};
  }
  return {};
}

/*
// Traverse FlatTree parent first backward.
// It looks mirror of Next().
static Node* Previous(const Node& node) {
  if (FlatTreeTraversal::FirstChild(node))
    return FlatTreeTraversal::LastWithin(node);
  return FlatTreeTraversal::PreviousSkippingChildren(node);
}

static Node* ComputeNodeBeforePosition(const PositionInFlatTree& position) {
  if (!position.AnchorNode())
    return nullptr;
  const Node& anchor = *position.AnchorNode();
  switch (position.AnchorType()) {
    case PositionAnchorType::kBeforeChildren:
      return Previous(anchor);
    case PositionAnchorType::kAfterChildren: {
      if (Node* last_child =
              FlatTreeTraversal::LastChild(anchor))
        return last_child;
      return Previous(anchor);
    }
    case PositionAnchorType::kOffsetInAnchor: {
      if (position.AnchorNode()->IsCharacterDataNode())
        return Previous(anchor);
      if (position.OffsetInContainerNode() == 0)
        return FlatTreeTraversal::PreviousSkippingChildren(anchor);
      Node* child_before_offset = FlatTreeTraversal::ChildAt(
        anchor, position.OffsetInContainerNode() - 1);
      return child_before_offset;
    }
    case PositionAnchorType::kBeforeAnchor:
      return Previous(anchor);
    case PositionAnchorType::kAfterAnchor:
      return position.AnchorNode();
  }
  NOTREACHED();
  return 0;
}

static PositionInFlatTree LastLayoutPosition(const PositionInFlatTree& end) {
  if (end.AnchorNode()->IsTextNode() && end.AnchorNode()->GetLayoutObject()) {
    return end;
  }

  for (Node* runner = ComputeNodeBeforePosition(end); runner;
       runner = Previous(*runner)) {
    if (runner->GetLayoutObject())
      return { runner, PositionAnchorType::kAfterAnchor };
  }
  return {};
}
*/

#define MYDEBUG

static EphemeralRangeInFlatTree CalcSelection(
    const FrameSelection& frame_selection) {
  const SelectionInDOMTree& selection_in_dom =
      frame_selection.GetSelectionInDOMTree();

#ifdef MYDEBUG
  EphemeralRangeInFlatTree debug_range;
  const PositionInFlatTree& debug_base =
      CreateVisiblePosition(ToPositionInFlatTree(selection_in_dom.Base()))
          .DeepEquivalent();
  const PositionInFlatTree& debug_extent =
      CreateVisiblePosition(ToPositionInFlatTree(selection_in_dom.Extent()))
          .DeepEquivalent();
  if (!debug_base.IsNull() && !debug_extent.IsNull() &&
      debug_base != debug_extent) {
    const bool base_is_first = debug_base.CompareTo(debug_extent) <= 0;
    const PositionInFlatTree& start = base_is_first ? debug_base : debug_extent;
    const PositionInFlatTree& end = base_is_first ? debug_extent : debug_base;
    debug_range = {MostForwardCaretPosition(start),
                   MostBackwardCaretPosition(end)};
  }
  DCHECK(debug_range.IsNull() || true);
#endif

  switch (ComputeSelectionMode(frame_selection)) {
    case SelectionMode::kNone:
      return {};
    case SelectionMode::kRange: {
      const PositionInFlatTree& base =
          ToPositionInFlatTree(selection_in_dom.Base());
      const PositionInFlatTree& extent =
          ToPositionInFlatTree(selection_in_dom.Extent());
      if (base.IsNull() || extent.IsNull())
        return {};
      const bool is_base_first = base <= extent;
      const PositionInFlatTree& start = is_base_first ? base : extent;
      const PositionInFlatTree& end = is_base_first ? extent : base;
      DCHECK_LE(start, end);
      const PositionInFlatTree start_position = FirstLayoutPosition(start);
      const PositionInFlatTree end_position = MostBackwardCaretPosition(
          CreateVisiblePosition(end).DeepEquivalent());
      if (start_position.IsNull() || end_position.IsNull() ||
          start_position >= end_position)
        return {};
      return {start_position, end_position};
    }
    case SelectionMode::kBlockCursor: {
      const PositionInFlatTree& base =
          CreateVisiblePosition(ToPositionInFlatTree(selection_in_dom.Base()))
              .DeepEquivalent();
      const PositionInFlatTree end_position =
          NextPositionOf(base, PositionMoveType::kGraphemeCluster);
      const VisibleSelectionInFlatTree& block_cursor =
          CreateVisibleSelection(SelectionInFlatTree::Builder()
                                     .SetBaseAndExtent(base, end_position)
                                     .Build());
      return {block_cursor.Start(), block_cursor.End()};
    }
  }
  NOTREACHED();
  return {};
}

// Objects each have a single selection rect to examine.
using SelectedObjectMap = HashMap<LayoutObject*, SelectionState>;
// Blocks contain selected objects and fill gaps between them, either on the
// left, right, or in between lines and blocks.
// In order to get the visual rect right, we have to examine left, middle, and
// right rects individually, since otherwise the union of those rects might
// remain the same even when changes have occurred.
using SelectedBlockMap = HashMap<LayoutBlock*, SelectionState>;
struct SelectedMap {
  STACK_ALLOCATED();
  SelectedObjectMap object_map;
  SelectedBlockMap block_map;

  SelectedMap() = default;
  SelectedMap(SelectedMap&& other) {
    object_map = std::move(other.object_map);
    block_map = std::move(other.block_map);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(SelectedMap);
};

static SelectedMap CollectSelectedMap(const SelectionPaintRange& range) {
  if (range.IsNull())
    return SelectedMap();

  SelectedMap selected_map;

  for (LayoutObject* runner : range) {
    if (runner->GetSelectionState() == SelectionState::kNone)
      continue;

    // Blocks are responsible for painting line gaps and margin gaps.  They
    // must be examined as well.
    selected_map.object_map.Set(runner, runner->GetSelectionState());
    for (LayoutBlock* containing_block = runner->ContainingBlock();
         containing_block && !containing_block->IsLayoutView();
         containing_block = containing_block->ContainingBlock()) {
      SelectedBlockMap::AddResult result = selected_map.block_map.insert(
          containing_block, containing_block->GetSelectionState());
      if (!result.is_new_entry)
        break;
    }
  }
  return selected_map;
}

// Update the selection status of all LayoutObjects between |start| and |end|.
static void SetSelectionState(const SelectionPaintRange& range) {
  if (range.IsNull())
    return;

  if (range.StartLayoutObject() == range.EndLayoutObject()) {
    range.StartLayoutObject()->SetSelectionStateIfNeeded(
        SelectionState::kStartAndEnd);
  } else {
    range.StartLayoutObject()->SetSelectionStateIfNeeded(
        SelectionState::kStart);
    range.EndLayoutObject()->SetSelectionStateIfNeeded(SelectionState::kEnd);
  }

  for (LayoutObject* runner : range) {
    if (runner != range.StartLayoutObject() &&
        runner != range.EndLayoutObject() && runner->CanBeSelectionLeaf())
      runner->SetSelectionStateIfNeeded(SelectionState::kInside);
  }
}

// Set SetSelectionState and ShouldInvalidateSelection flag of LayoutObjects
// comparing them in |new_range| and |old_range|.
static void UpdateLayoutObjectState(const SelectionPaintRange& new_range,
                                    const SelectionPaintRange& old_range) {
  const SelectedMap& old_selected_map = CollectSelectedMap(old_range);

  // Now clear the selection.
  for (auto layout_object : old_selected_map.object_map.Keys())
    layout_object->SetSelectionStateIfNeeded(SelectionState::kNone);

  SetSelectionState(new_range);

  // Now that the selection state has been updated for the new objects, walk
  // them again and put them in the new objects list.
  // TODO(editing-dev): |new_selected_map| doesn't really need to store the
  // SelectionState, it's just more convenient to have it use the same data
  // structure as |old_selected_map|.
  SelectedMap new_selected_map = CollectSelectedMap(new_range);

  // Have any of the old selected objects changed compared to the new selection?
  for (const auto& pair : old_selected_map.object_map) {
    LayoutObject* obj = pair.key;
    SelectionState new_selection_state = obj->GetSelectionState();
    SelectionState old_selection_state = pair.value;
    if (new_selection_state != old_selection_state ||
        (new_range.StartLayoutObject() == obj &&
         new_range.StartOffset() != old_range.StartOffset()) ||
        (new_range.EndLayoutObject() == obj &&
         new_range.EndOffset() != old_range.EndOffset())) {
      obj->SetShouldInvalidateSelection();
      new_selected_map.object_map.erase(obj);
    }
  }

  // Any new objects that remain were not found in the old objects dict, and so
  // they need to be updated.
  for (auto layout_object : new_selected_map.object_map.Keys())
    layout_object->SetShouldInvalidateSelection();

  // Have any of the old blocks changed?
  for (const auto& pair : old_selected_map.block_map) {
    LayoutBlock* block = pair.key;
    SelectionState new_selection_state = block->GetSelectionState();
    SelectionState old_selection_state = pair.value;
    if (new_selection_state != old_selection_state) {
      block->SetShouldInvalidateSelection();
      new_selected_map.block_map.erase(block);
    }
  }

  // Any new blocks that remain were not found in the old blocks dict, and so
  // they need to be updated.
  for (auto layout_object : new_selected_map.block_map.Keys())
    layout_object->SetShouldInvalidateSelection();
}

std::pair<int, int> LayoutSelection::SelectionStartEnd() {
  Commit();
  if (paint_range_.IsNull())
    return std::make_pair(-1, -1);
  return std::make_pair(paint_range_.StartOffset(), paint_range_.EndOffset());
}

void LayoutSelection::ClearSelection() {
  // For querying Layer::compositingState()
  // This is correct, since destroying layout objects needs to cause eager paint
  // invalidations.
  DisableCompositingQueryAsserts disabler;

  // Just return if the selection is already empty.
  if (paint_range_.IsNull())
    return;

  for (auto layout_object : paint_range_) {
    const SelectionState old_state = layout_object->GetSelectionState();
    layout_object->SetSelectionStateIfNeeded(SelectionState::kNone);
    if (layout_object->GetSelectionState() == old_state)
      continue;
    layout_object->SetShouldInvalidateSelection();
  }

  // Reset selection.
  paint_range_ = SelectionPaintRange();
}

static SelectionPaintRange CalcSelectionPaintRange(
    const FrameSelection& frame_selection) {
  const SelectionInDOMTree& selection_in_dom =
      frame_selection.GetSelectionInDOMTree();
  if (selection_in_dom.IsNone())
    return SelectionPaintRange();

  const EphemeralRangeInFlatTree& selection = CalcSelection(frame_selection);
  if (selection.IsCollapsed() || frame_selection.IsHidden())
    return SelectionPaintRange();

  const PositionInFlatTree start_pos = selection.StartPosition();
  const PositionInFlatTree end_pos = selection.EndPosition();
  DCHECK_LE(start_pos, end_pos);
  LayoutObject* start_layout_object = start_pos.AnchorNode()->GetLayoutObject();
  LayoutObject* end_layout_object = end_pos.AnchorNode()->GetLayoutObject();
  DCHECK(start_layout_object);
  DCHECK(end_layout_object);
  DCHECK(start_layout_object->View() == end_layout_object->View());
  if (!start_layout_object || !end_layout_object)
    return SelectionPaintRange();

  return SelectionPaintRange(start_layout_object,
                             start_pos.ComputeEditingOffset(),
                             end_layout_object, end_pos.ComputeEditingOffset());
}

void LayoutSelection::Commit() {
  if (!HasPendingSelection())
    return;
  has_pending_selection_ = false;

  const SelectionPaintRange& new_range =
      CalcSelectionPaintRange(*frame_selection_);
  if (new_range.IsNull()) {
    ClearSelection();
    return;
  }
  // Just return if the selection hasn't changed.
  if (paint_range_ == new_range)
    return;

  DCHECK(frame_selection_->GetDocument().GetLayoutView()->GetFrameView());
  DCHECK(!frame_selection_->GetDocument().NeedsLayoutTreeUpdate());
  UpdateLayoutObjectState(new_range, paint_range_);
  paint_range_ = new_range;
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

  // Create a single bounding box rect that encloses the whole selection.
  LayoutRect selected_rect;
  const SelectedMap& current_map = CollectSelectedMap(paint_range_);
  for (auto layout_object : current_map.object_map.Keys())
    selected_rect.Unite(SelectionRectForLayoutObject(layout_object));
  for (auto layout_block : current_map.block_map.Keys())
    selected_rect.Unite(SelectionRectForLayoutObject(layout_block));

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

}  // namespace blink
