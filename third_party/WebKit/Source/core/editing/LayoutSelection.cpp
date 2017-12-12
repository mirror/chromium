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
#include "core/html/forms/TextControlElement.h"
#include "core/layout/LayoutText.h"
#include "core/layout/LayoutTextFragment.h"
#include "core/layout/LayoutView.h"
#include "core/paint/PaintLayer.h"

namespace blink {

SelectionPaintRange::SelectionPaintRange(LayoutObject* start_layout_object,
                                         WTF::Optional<unsigned> start_offset,
                                         LayoutObject* end_layout_object,
                                         WTF::Optional<unsigned> end_offset)
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
  return start_layout_object_;
}

WTF::Optional<unsigned> SelectionPaintRange::StartOffset() const {
  return start_offset_;
}

LayoutObject* SelectionPaintRange::EndLayoutObject() const {
  return end_layout_object_;
}

WTF::Optional<unsigned> SelectionPaintRange::EndOffset() const {
  return end_offset_;
}

LayoutSelection::LayoutSelection(FrameSelection& frame_selection)
    : frame_selection_(&frame_selection),
      has_pending_selection_(false),
      paint_range_() {}

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

static EphemeralRangeInFlatTree CalcSelectionInFlatTreeInternal(
    const FrameSelection& frame_selection) {
  const SelectionInDOMTree& selection_in_dom =
      frame_selection.GetSelectionInDOMTree();
  if (selection_in_dom.IsNone())
    return {};
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
static SelectionInFlatTree CalcSelectionInFlatTree(
    const FrameSelection& frame_selection) {
  const EphemeralRangeInFlatTree& selection =
      CalcSelectionInFlatTreeInternal(frame_selection);
  if (selection.IsNull() || frame_selection.IsHidden())
    return {};
  return SelectionInFlatTree::Builder()
      .SetAsForwardSelection(selection)
      .Build();
}

// LayoutObjects each has SelectionState of kStart, kEnd, kStartAndEnd, or
// kInside.
using SelectedLayoutObjects = HashSet<LayoutObject*>;
// OldSelectedLayoutObjects is current selected LayoutObjects with
// current SelectionState which is kStart, kEnd, kStartAndEnd or kInside.
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

void PrintSelectionStateInLayoutView(const FrameSelection& selection) {
  std::stringstream stream;
  stream << std::endl << "layout_objects:" << std::endl;
  LayoutView* layout_view = selection.GetDocument().GetLayoutView();
  for (LayoutObject* layout_object = layout_view; layout_object;
       layout_object = layout_object->NextInPreOrder()) {
    PrintLayoutObjectForSelection(stream, layout_object);
    stream << std::endl;
  }
  LOG(INFO) << stream.str();
}
#endif

// This class represents a selection range in layout tree and each LayoutObject
// is SelectionState-marked.
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

enum ComputePaintRangeOption {
  kMarkingSelection,
  kClearSelection,
  kSelectionBounds,
};

// This class represents a selection range in layout tree and each LayoutObject
// is SelectionState-marked.
class ComputePaintRangeResult {
  STACK_ALLOCATED();

 public:
  ComputePaintRangeResult() = default;
  ComputePaintRangeResult(ComputePaintRangeResult&& other) {
    option = other.option;
    paint_range = other.paint_range;
    selected_objects = std::move(other.selected_objects);
    old_selected_objects = std::move(other.old_selected_objects);
    containing_block_set = std::move(other.containing_block_set);
    selected_rect = other.selected_rect;
  }

  ComputePaintRangeOption option;
  SelectionPaintRange paint_range;
  SelectedLayoutObjects selected_objects;
  OldSelectedLayoutObjects old_selected_objects;
  HashSet<LayoutObject*> containing_block_set;
  LayoutRect selected_rect;

 private:
  DISALLOW_COPY_AND_ASSIGN(ComputePaintRangeResult);
};

static ComputePaintRangeResult ComputePaintRange(const SelectionInFlatTree&,
                                                 ComputePaintRangeOption);

static void SetShouldInvalidateIfNeeded(LayoutObject* layout_object) {
  if (layout_object->ShouldInvalidateSelection())
    return;
  layout_object->SetShouldInvalidateSelection();

  // We should invalidate if ancestor of |layout_object| is LayoutSVGText
  // because SVGRootInlineBoxPainter::Paint() paints selection for
  // |layout_object| in/ LayoutSVGText and it is invoked when parent
  // LayoutSVGText is invalidated.
  // That is different from InlineTextBoxPainter::Paint() which paints
  // LayoutText selection when LayoutText is invalidated.
  if (!layout_object->IsSVG())
    return;
  for (LayoutObject* parent = layout_object->Parent(); parent;
       parent = parent->Parent()) {
    if (parent->IsSVGRoot())
      return;
    if (parent->IsSVGText()) {
      if (!parent->ShouldInvalidateSelection())
        parent->SetShouldInvalidateSelection();
      return;
    }
  }
}

static void SetSelectionStateIfNeeded(LayoutObject* layout_object,
                                      SelectionState state) {
  DCHECK_NE(state, SelectionState::kContain) << layout_object;
  DCHECK_NE(state, SelectionState::kNone) << layout_object;
  if (layout_object->GetSelectionState() == state)
    return;
  // TODO(yoichio): Once we make LayoutObject::SetSelectionState() tribial, use
  // it directly.
  layout_object->LayoutObject::SetSelectionState(state);

  // Set containing block SelectionState kContain for CSS ::selection style.
  // See LayoutObject::InvalidatePaintForSelection().
  for (LayoutObject* containing_block = layout_object->ContainingBlock();
       containing_block;
       containing_block = containing_block->ContainingBlock()) {
    if (containing_block->GetSelectionState() == SelectionState::kContain)
      return;
    containing_block->LayoutObject::SetSelectionState(SelectionState::kContain);
  }
}

// Set ShouldInvalidateSelection flag of LayoutObjects
// comparing them in |new_range| and |old_range|.
static void SetShouldInvalidateSelection(
    const ComputePaintRangeResult& new_result,
    const ComputePaintRangeResult& old_result) {
  const NewPaintRangeAndSelectedLayoutObjects new_range = {
      new_result.paint_range, new_result.selected_objects};
  const SelectionPaintRange& old_range = old_result.paint_range;
  const OldSelectedLayoutObjects& old_selected_objects =
      old_result.old_selected_objects;
  // We invalidate each LayoutObject in new SelectionPaintRange which
  // has SelectionState of kStart, kEnd, kStartAndEnd, or kInside
  // and is not in old SelectionPaintRange.
  for (LayoutObject* layout_object : new_range.LayoutObjects()) {
    if (old_selected_objects.Contains(layout_object))
      continue;
    const SelectionState new_state = layout_object->GetSelectionState();
    DCHECK_NE(new_state, SelectionState::kContain) << layout_object;
    DCHECK_NE(new_state, SelectionState::kNone) << layout_object;
    SetShouldInvalidateIfNeeded(layout_object);
  }
  // For LayoutObject in old SelectionPaintRange, we invalidate LayoutObjects
  // each of:
  // 1. LayoutObject was painted and would not be painted.
  // 2. LayoutObject was not painted and would be painted.
  for (const auto& key_value : old_selected_objects) {
    LayoutObject* const layout_object = key_value.key;
    const SelectionState old_state = key_value.value;
    const SelectionState new_state = layout_object->GetSelectionState();
    if (new_state == old_state)
      continue;
    DCHECK(new_state != SelectionState::kNone ||
           old_state != SelectionState::kNone)
        << layout_object;
    DCHECK_NE(new_state, SelectionState::kContain) << layout_object;
    DCHECK_NE(old_state, SelectionState::kContain) << layout_object;
    SetShouldInvalidateIfNeeded(layout_object);
  }

  // Invalidate Selection start/end is moving on a same node.
  const SelectionPaintRange& new_paint_range = new_range.PaintRange();
  if (new_paint_range.IsNull() || old_range.IsNull())
    return;
  if (new_paint_range.StartLayoutObject()->IsText() &&
      new_paint_range.StartLayoutObject() == old_range.StartLayoutObject() &&
      new_paint_range.StartOffset() != old_range.StartOffset())
    SetShouldInvalidateIfNeeded(new_paint_range.StartLayoutObject());
  if (new_paint_range.EndLayoutObject()->IsText() &&
      new_paint_range.EndLayoutObject() == old_range.EndLayoutObject() &&
      new_paint_range.EndOffset() != old_range.EndOffset())
    SetShouldInvalidateIfNeeded(new_paint_range.EndLayoutObject());
}

WTF::Optional<unsigned> LayoutSelection::SelectionStart() const {
  DCHECK(!HasPendingSelection());
  return start_offset_;
}

WTF::Optional<unsigned> LayoutSelection::SelectionEnd() const {
  DCHECK(!HasPendingSelection());
  return end_offset_;
}

void LayoutSelection::ClearSelection() {
  // For querying Layer::compositingState()
  // This is correct, since destroying layout objects needs to cause eager paint
  // invalidations.
  DisableCompositingQueryAsserts disabler;

  // Just return if the selection is already empty.
  if (paint_range_.IsNone())
    return;

  const ComputePaintRangeResult& result =
      ComputePaintRange(paint_range_, ComputePaintRangeOption::kClearSelection);
  for (LayoutObject* const layout_object : result.old_selected_objects.Keys())
    SetShouldInvalidateIfNeeded(layout_object);

  // Reset selection.
  paint_range_ = {};
  start_offset_ = WTF::nullopt;
  end_offset_ = WTF::nullopt;
}

static WTF::Optional<unsigned> ComputeStartOffset(
    const LayoutObject& layout_object,
    const PositionInFlatTree& position) {
  Node* const layout_node = layout_object.GetNode();
  if (!layout_node || !layout_node->IsTextNode())
    return WTF::nullopt;

  if (layout_node == position.AnchorNode())
    return position.OffsetInContainerNode();
  return 0;
}

static WTF::Optional<unsigned> ComputeEndOffset(
    const LayoutObject& layout_object,
    const PositionInFlatTree& position) {
  Node* const layout_node = layout_object.GetNode();
  if (!layout_node || !layout_node->IsTextNode())
    return WTF::nullopt;

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

static void SetSelectionNone(OldSelectedLayoutObjects& old_selected_objects,
                             HashSet<LayoutObject*>& containing_block_set,
                             LayoutObject* layout_object) {
  const SelectionState old_state = layout_object->GetSelectionState();
  if (old_state == SelectionState::kNone)
    return;
  if (old_state != SelectionState::kContain)
    old_selected_objects.insert(layout_object, old_state);
  // TODO(yoichio): Once we make LayoutObject::SetSelectionState() trivial,
  // use it directly.
  layout_object->LayoutObject::SetSelectionState(SelectionState::kNone);

  // Reset containing block SelectionState for CSS ::selection style.
  // See LayoutObject::InvalidatePaintForSelection().
  for (LayoutObject* containing_block = layout_object->ContainingBlock();
       containing_block;
       containing_block = containing_block->ContainingBlock()) {
    if (containing_block_set.Contains(containing_block))
      break;
    containing_block->LayoutObject::SetSelectionState(SelectionState::kNone);
    containing_block_set.insert(containing_block);
  }
}

static LayoutRect SelectionRectForLayoutObject(const LayoutObject*);
static void MarkSelected(ComputePaintRangeResult& selected_objects,
                         LayoutObject* layout_object,
                         SelectionState state) {
  switch (selected_objects.option) {
    case ComputePaintRangeOption::kMarkingSelection: {
      DCHECK(layout_object->CanBeSelectionLeaf());
      SetSelectionStateIfNeeded(layout_object, state);
      selected_objects.selected_objects.insert(layout_object);
      return;
    }
    case ComputePaintRangeOption::kClearSelection: {
      SetSelectionNone(selected_objects.old_selected_objects,
                       selected_objects.containing_block_set, layout_object);
      return;
    }
    case ComputePaintRangeOption::kSelectionBounds: {
      // Create a single bounding box rect that encloses the whole selection.
      const SelectionState state = layout_object->GetSelectionState();
      if (state == SelectionState::kContain || state == SelectionState::kNone)
        return;
      selected_objects.selected_rect.Unite(
          SelectionRectForLayoutObject(layout_object));
      return;
    }
  }
  NOTREACHED();
}

static void MarkSelectedInside(ComputePaintRangeResult& selected_objects,
                               LayoutObject* layout_object) {
  MarkSelected(selected_objects, layout_object, SelectionState::kInside);
  LayoutTextFragment* const first_letter_part =
      FirstLetterPartFor(layout_object);
  if (!first_letter_part)
    return;
  MarkSelected(selected_objects, first_letter_part, SelectionState::kInside);
}

static void MarkStartAndEndInOneNode(ComputePaintRangeResult& selected_objects,
                                     LayoutObject* layout_object,
                                     WTF::Optional<unsigned> start_offset,
                                     WTF::Optional<unsigned> end_offset) {
  if (!layout_object->GetNode()->IsTextNode()) {
    DCHECK(!start_offset.has_value());
    DCHECK(!end_offset.has_value());
    MarkSelected(selected_objects, layout_object, SelectionState::kStartAndEnd);
    selected_objects.paint_range = {layout_object, WTF::nullopt, layout_object,
                                    WTF::nullopt};
    return;
  }

  DCHECK(start_offset.has_value());
  DCHECK(end_offset.has_value());
  DCHECK_GE(end_offset.value(), start_offset.value());
  if (start_offset.value() == end_offset.value()) {
    selected_objects.paint_range = {};
    return;
  }
  LayoutTextFragment* const first_letter_part =
      FirstLetterPartFor(layout_object);
  if (!first_letter_part) {
    MarkSelected(selected_objects, layout_object, SelectionState::kStartAndEnd);
    selected_objects.paint_range = {layout_object, start_offset, layout_object,
                                    end_offset};
    return;
  }
  const unsigned unsigned_start = start_offset.value();
  const unsigned unsigned_end = end_offset.value();
  LayoutTextFragment* const remaining_part =
      ToLayoutTextFragment(layout_object);
  if (unsigned_start >= remaining_part->Start()) {
    // Case 1: The selection starts and ends in remaining part.
    DCHECK_GT(unsigned_end, remaining_part->Start());
    MarkSelected(selected_objects, remaining_part,
                 SelectionState::kStartAndEnd);
    selected_objects.paint_range = {
        remaining_part, unsigned_start - remaining_part->Start(),
        remaining_part, unsigned_end - remaining_part->Start()};
    return;
  }
  if (unsigned_end <= remaining_part->Start()) {
    // Case 2: The selection starts and ends in first letter part.
    MarkSelected(selected_objects, first_letter_part,
                 SelectionState::kStartAndEnd);
    selected_objects.paint_range = {first_letter_part, start_offset,
                                    first_letter_part, end_offset};
    return;
  }
  // Case 3: The selection starts in first-letter part and ends in remaining
  // part.
  DCHECK_GT(unsigned_end, remaining_part->Start());
  MarkSelected(selected_objects, first_letter_part, SelectionState::kStart);
  MarkSelected(selected_objects, remaining_part, SelectionState::kEnd);
  selected_objects.paint_range = {first_letter_part, start_offset,
                                  remaining_part,
                                  unsigned_end - remaining_part->Start()};
  return;
}

// LayoutObjectAndOffset represents start or end of SelectionPaintRange.
struct LayoutObjectAndOffset {
  STACK_ALLOCATED();
  LayoutObject* layout_object;
  WTF::Optional<unsigned> offset;

  explicit LayoutObjectAndOffset(LayoutObject* passed_layout_object)
      : layout_object(passed_layout_object), offset(WTF::nullopt) {
    DCHECK(passed_layout_object);
    DCHECK(!passed_layout_object->GetNode()->IsTextNode());
  }
  LayoutObjectAndOffset(LayoutText* layout_text, unsigned passed_offset)
      : layout_object(layout_text), offset(passed_offset) {
    DCHECK(layout_object);
  }
};

LayoutObjectAndOffset MarkStart(ComputePaintRangeResult& selected_objects,
                                LayoutObject* start_layout_object,
                                WTF::Optional<unsigned> start_offset) {
  if (!start_layout_object->GetNode()->IsTextNode()) {
    DCHECK(!start_offset.has_value());
    MarkSelected(selected_objects, start_layout_object, SelectionState::kStart);
    return LayoutObjectAndOffset(start_layout_object);
  }

  DCHECK(start_offset.has_value());
  const unsigned unsigned_offset = start_offset.value();
  LayoutText* const start_layout_text = ToLayoutText(start_layout_object);
  if (unsigned_offset >= start_layout_text->TextStartOffset()) {
    // |start_offset| is within |start_layout_object| whether it has first
    // letter part or not.
    MarkSelected(selected_objects, start_layout_object, SelectionState::kStart);
    return {start_layout_text,
            unsigned_offset - start_layout_text->TextStartOffset()};
  }

  // |start_layout_object| has first letter part and |start_offset| is within
  // the part.
  LayoutTextFragment* const first_letter_part =
      FirstLetterPartFor(start_layout_object);
  DCHECK(first_letter_part);
  MarkSelected(selected_objects, first_letter_part, SelectionState::kStart);
  MarkSelected(selected_objects, start_layout_text, SelectionState::kInside);
  return {first_letter_part, start_offset.value()};
}

LayoutObjectAndOffset MarkEnd(ComputePaintRangeResult& result,
                              LayoutObject* end_layout_object,
                              WTF::Optional<unsigned> end_offset) {
  if (!end_layout_object->GetNode()->IsTextNode()) {
    DCHECK(!end_offset.has_value());
    MarkSelected(result, end_layout_object, SelectionState::kEnd);
    return LayoutObjectAndOffset(end_layout_object);
  }

  DCHECK(end_offset.has_value());
  const unsigned unsigned_offset = end_offset.value();
  LayoutText* const end_layout_text = ToLayoutText(end_layout_object);
  if (unsigned_offset >= end_layout_text->TextStartOffset()) {
    // |end_offset| is within |end_layout_object| whether it has first
    // letter part or not.
    MarkSelected(result, end_layout_object, SelectionState::kEnd);
    if (LayoutTextFragment* const first_letter_part =
            FirstLetterPartFor(end_layout_object)) {
      MarkSelected(result, first_letter_part, SelectionState::kInside);
    }
    return {end_layout_text,
            unsigned_offset - end_layout_text->TextStartOffset()};
  }

  // |end_layout_object| has first letter part and |end_offset| is within
  // the part.
  LayoutTextFragment* const first_letter_part =
      FirstLetterPartFor(end_layout_object);
  DCHECK(first_letter_part);
  MarkSelected(result, first_letter_part, SelectionState::kEnd);
  return {first_letter_part, end_offset.value()};
}

static void MarkStartAndEndInTwoNodes(ComputePaintRangeResult& result,
                                      LayoutObject* start_layout_object,
                                      WTF::Optional<unsigned> start_offset,
                                      LayoutObject* end_layout_object,
                                      WTF::Optional<unsigned> end_offset) {
  const LayoutObjectAndOffset& start =
      MarkStart(result, start_layout_object, start_offset);
  const LayoutObjectAndOffset& end =
      MarkEnd(result, end_layout_object, end_offset);
  result.paint_range = {start.layout_object, start.offset, end.layout_object,
                        end.offset};
}

static ComputePaintRangeResult ComputePaintRange(
    const SelectionInFlatTree& selection_in_flat,
    ComputePaintRangeOption option) {
  const EphemeralRangeInFlatTree& selection = selection_in_flat.ComputeRange();
  if (selection.IsCollapsed())
    return {};

  ComputePaintRangeResult result;
  result.option = option;
  // Find first/last visible LayoutObject while
  // marking SelectionState and collecting invalidation candidate LayoutObjects.
  LayoutObject* start_layout_object = nullptr;
  LayoutObject* end_layout_object = nullptr;
  for (const Node& node : selection.Nodes()) {
    LayoutObject* const layout_object = node.GetLayoutObject();
    if (!layout_object ||
        (option == ComputePaintRangeOption::kMarkingSelection &&
         !layout_object->CanBeSelectionLeaf()))
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
      MarkSelectedInside(result, end_layout_object);
    end_layout_object = layout_object;
  }

  // No valid LayOutObject found.
  if (!start_layout_object) {
    DCHECK(!end_layout_object);
    return {};
  }

  // Compute offset. It has value iff start/end is text.
  const WTF::Optional<unsigned> start_offset = ComputeStartOffset(
      *start_layout_object, selection.StartPosition().ToOffsetInAnchor());
  const WTF::Optional<unsigned> end_offset = ComputeEndOffset(
      *end_layout_object, selection.EndPosition().ToOffsetInAnchor());

  if (start_layout_object == end_layout_object) {
    MarkStartAndEndInOneNode(result, start_layout_object, start_offset,
                             end_offset);
  } else {
    MarkStartAndEndInTwoNodes(result, start_layout_object, start_offset,
                              end_layout_object, end_offset);
  }
  return result;
}

void LayoutSelection::SetHasPendingSelection() {
  has_pending_selection_ = true;
}

static void CheckPaintRange(const SelectionPaintRange& paint_range) {
  if (paint_range.IsNull())
    return;
  // TODO(yoichio): Remove this if state.
  // This SelectionState reassignment is ad-hoc patch for
  // prohibiting use-after-free(crbug.com/752715).
  // LayoutText::setSelectionState(state) propergates |state| to ancestor
  // LayoutObjects, which can accidentally change start/end LayoutObject state
  // then LayoutObject::IsSelectionBorder() returns false although we should
  // clear selection at LayoutObject::WillBeRemoved().
  // We should make LayoutObject::setSelectionState() trivial and remove
  // such propagation or at least do it in LayoutSelection.
  if ((paint_range.StartLayoutObject()->GetSelectionState() !=
           SelectionState::kStart &&
       paint_range.StartLayoutObject()->GetSelectionState() !=
           SelectionState::kStartAndEnd) ||
      (paint_range.EndLayoutObject()->GetSelectionState() !=
           SelectionState::kEnd &&
       paint_range.EndLayoutObject()->GetSelectionState() !=
           SelectionState::kStartAndEnd)) {
    if (paint_range.StartLayoutObject() == paint_range.EndLayoutObject()) {
      paint_range.StartLayoutObject()->LayoutObject::SetSelectionState(
          SelectionState::kStartAndEnd);
    } else {
      paint_range.StartLayoutObject()->LayoutObject::SetSelectionState(
          SelectionState::kStart);
      paint_range.EndLayoutObject()->LayoutObject::SetSelectionState(
          SelectionState::kEnd);
    }
  }
  // TODO(yoichio): If start == end, they should be kStartAndEnd.
  // If not, start.SelectionState == kStart and vice versa.
  DCHECK(paint_range.StartLayoutObject()->GetSelectionState() ==
             SelectionState::kStart ||
         paint_range.StartLayoutObject()->GetSelectionState() ==
             SelectionState::kStartAndEnd);
  DCHECK(paint_range.EndLayoutObject()->GetSelectionState() ==
             SelectionState::kEnd ||
         paint_range.EndLayoutObject()->GetSelectionState() ==
             SelectionState::kStartAndEnd);
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

  const ComputePaintRangeResult& old_result =
      ComputePaintRange(paint_range_, ComputePaintRangeOption::kClearSelection);
  const SelectionInFlatTree& new_selection =
      CalcSelectionInFlatTree(*frame_selection_);
  const ComputePaintRangeResult& result = ComputePaintRange(
      new_selection, ComputePaintRangeOption::kMarkingSelection);

  DCHECK(frame_selection_->GetDocument().GetLayoutView()->GetFrameView());
  SetShouldInvalidateSelection(result, old_result);

  paint_range_ = new_selection;
  start_offset_ = result.paint_range.StartOffset();
  end_offset_ = result.paint_range.EndOffset();
  CheckPaintRange(result.paint_range);
}

void LayoutSelection::OnDocumentShutdown() {
  has_pending_selection_ = false;
  paint_range_ = {};
  start_offset_ = WTF::nullopt;
  end_offset_ = WTF::nullopt;
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
  if (paint_range_.IsNone())
    return IntRect();

  const ComputePaintRangeResult& result = ComputePaintRange(
      paint_range_, ComputePaintRangeOption::kSelectionBounds);

  return PixelSnappedIntRect(result.selected_rect);
}

void LayoutSelection::InvalidatePaintForSelection() {
  if (paint_range_.IsNone())
    return;

  ComputePaintRange(paint_range_, ComputePaintRangeOption::kClearSelection);
}

void LayoutSelection::Trace(blink::Visitor* visitor) {
  visitor->Trace(frame_selection_);
  visitor->Trace(paint_range_);
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
