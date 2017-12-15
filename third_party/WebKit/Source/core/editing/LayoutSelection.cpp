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
#include "core/layout/ng/inline/ng_offset_mapping.h"
#include "core/layout/ng/inline/ng_physical_text_fragment.h"
#include "core/paint/PaintLayer.h"

namespace blink {

// This represents a selection range in layout tree for painting.
// The current selection to be painted is represented as 2 pairs of
// (LayoutObject, offset).
// 2 LayoutObjects are only valid for |Text| node without 'transform' or
// 'first-letter'.
// TODO(editing-dev): Clarify the meaning of "offset".
// editing/ passes them as offsets in the DOM tree but layout uses them as
// offset in the layout tree. This doesn't work in the cases of
// CSS first-letter or character transform. See crbug.com/17528.
struct SelectionPaintRange {
  STACK_ALLOCATED();

  LayoutObject* start_layout_object = nullptr;
  WTF::Optional<unsigned> start_offset = WTF::nullopt;
  LayoutObject* end_layout_object = nullptr;
  WTF::Optional<unsigned> end_offset = WTF::nullopt;

  SelectionPaintRange() = default;
  SelectionPaintRange(LayoutObject* passed_start_layout_object,
    WTF::Optional<unsigned> passed_start_offset,
    LayoutObject* passed_end_layout_object,
    WTF::Optional<unsigned> passed_end_offset)
    : start_layout_object(passed_start_layout_object),
    start_offset(passed_start_offset),
    end_layout_object(passed_end_layout_object),
    end_offset(passed_end_offset) {}

  bool IsNull() const { return !start_layout_object; }
};

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

struct LayoutObjectAndOffset;
class TraversePaintRange {
  STATIC_ONLY(TraversePaintRange);

public:
  struct MarkingResult {
    SelectionPaintRange paint_range;
    SelectedLayoutObjects selected_objects;
  };
  static MarkingResult MarkingSelection(const SelectionInFlatTree&) {
    return {};
  }
  struct ClearResult {
    SelectionPaintRange paint_range;
    OldSelectedLayoutObjects old_selected_objects;
  };
  static ClearResult ClearSelection(const SelectionInFlatTree&) {
    return {};
  }
  struct BoundsResult {};
  static BoundsResult BoundSelection(const SelectionInFlatTree&) {
    return {};
  }
private:
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
    void MarkSelected(LayoutObject*, SelectionState);
    void MarkSelectedInside(LayoutObject*);
    void MarkStartAndEndInOneNode(LayoutObject*,
      WTF::Optional<unsigned> start_offset,
      WTF::Optional<unsigned> end_offset);
    LayoutObjectAndOffset TraversePaintRange::ComputePaintRangeResult::MarkStart(
      LayoutObject*, WTF::Optional<unsigned>);
    LayoutObjectAndOffset TraversePaintRange::ComputePaintRangeResult::MarkEnd(
      LayoutObject*, WTF::Optional<unsigned>);
    void MarkStartAndEndInTwoNodes(
      LayoutObject* start_layout_object,
      WTF::Optional<unsigned> start_offset,
      LayoutObject* end_layout_object,
      WTF::Optional<unsigned> end_offset);
    void ComputeNewPaintRange(LayoutObject* start_layout_object,
      WTF::Optional<unsigned> start_node_offset,
      LayoutObject* end_layout_object,
      WTF::Optional<unsigned> end_node_offset);

    DISALLOW_COPY_AND_ASSIGN(ComputePaintRangeResult);
  };

  static ComputePaintRangeResult ComputePaintRange(const SelectionInFlatTree&,
    ComputePaintRangeOption);
};

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
    const TraversePaintRange::MarkingResult& new_result,
    const TraversePaintRange::ClearResult& old_result) {
  const SelectionPaintRange& old_range = old_result.paint_range;
  const OldSelectedLayoutObjects& old_selected_objects =
      old_result.old_selected_objects;
  // We invalidate each LayoutObject in new SelectionPaintRange which
  // has SelectionState of kStart, kEnd, kStartAndEnd, or kInside
  // and is not in old SelectionPaintRange.
  for (LayoutObject* layout_object : new_result.selected_objects) {
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
  const SelectionPaintRange& new_paint_range = new_result.paint_range;
  if (new_paint_range.IsNull() || old_range.IsNull())
    return;
  if (new_paint_range.start_layout_object->IsText() &&
      new_paint_range.start_layout_object == old_range.start_layout_object &&
      new_paint_range.start_offset != old_range.start_offset)
    SetShouldInvalidateIfNeeded(new_paint_range.start_layout_object);
  if (new_paint_range.end_layout_object->IsText() &&
      new_paint_range.end_layout_object == old_range.end_layout_object &&
      new_paint_range.end_offset != old_range.end_offset)
    SetShouldInvalidateIfNeeded(new_paint_range.end_layout_object);
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

  const TraversePaintRange::ClearResult& result =
    TraversePaintRange::ClearSelection(paint_range_);
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
void TraversePaintRange::ComputePaintRangeResult::MarkSelected(
                         LayoutObject* layout_object,
                         SelectionState state) {
  switch (option) {
    case ComputePaintRangeOption::kMarkingSelection: {
      DCHECK(layout_object->CanBeSelectionLeaf());
      SetSelectionStateIfNeeded(layout_object, state);
      selected_objects.insert(layout_object);
      return;
    }
    case ComputePaintRangeOption::kClearSelection: {
      SetSelectionNone(old_selected_objects,
                       containing_block_set, layout_object);
      return;
    }
    case ComputePaintRangeOption::kSelectionBounds: {
      // Create a single bounding box rect that encloses the whole selection.
      const SelectionState state = layout_object->GetSelectionState();
      if (state == SelectionState::kContain || state == SelectionState::kNone)
        return;
      selected_rect.Unite(
          SelectionRectForLayoutObject(layout_object));
      return;
    }
  }
  NOTREACHED();
}

void TraversePaintRange::ComputePaintRangeResult::MarkSelectedInside(
                               LayoutObject* layout_object) {
  MarkSelected(layout_object, SelectionState::kInside);
  LayoutTextFragment* const first_letter_part =
      FirstLetterPartFor(layout_object);
  if (!first_letter_part)
    return;
  MarkSelected(first_letter_part, SelectionState::kInside);
}

void TraversePaintRange::ComputePaintRangeResult::MarkStartAndEndInOneNode(
                                     LayoutObject* layout_object,
                                     WTF::Optional<unsigned> start_offset,
                                     WTF::Optional<unsigned> end_offset) {
  if (!layout_object->GetNode()->IsTextNode()) {
    DCHECK(!start_offset.has_value());
    DCHECK(!end_offset.has_value());
    MarkSelected(layout_object, SelectionState::kStartAndEnd);
    paint_range = {layout_object, WTF::nullopt,
                                    layout_object, WTF::nullopt};
    return;
  }

  DCHECK(start_offset.has_value());
  DCHECK(end_offset.has_value());
  DCHECK_GE(end_offset.value(), start_offset.value());
  if (start_offset.value() == end_offset.value()) {
    paint_range = {};
    return;
  }
  LayoutTextFragment* const first_letter_part =
      FirstLetterPartFor(layout_object);
  if (!first_letter_part) {
    MarkSelected(layout_object, SelectionState::kStartAndEnd);
    paint_range = {layout_object, start_offset, layout_object,
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
    MarkSelected(remaining_part,
                 SelectionState::kStartAndEnd);
    paint_range = {
        remaining_part, unsigned_start - remaining_part->Start(),
        remaining_part, unsigned_end - remaining_part->Start()};
    return;
  }
  if (unsigned_end <= remaining_part->Start()) {
    // Case 2: The selection starts and ends in first letter part.
    MarkSelected(first_letter_part,
                 SelectionState::kStartAndEnd);
    paint_range = {first_letter_part, start_offset,
                                    first_letter_part, end_offset};
    return;
  }
  // Case 3: The selection starts in first-letter part and ends in remaining
  // part.
  DCHECK_GT(unsigned_end, remaining_part->Start());
  MarkSelected(first_letter_part, SelectionState::kStart);
  MarkSelected(remaining_part, SelectionState::kEnd);
  paint_range = {
      first_letter_part, start_offset,
      remaining_part, unsigned_end - remaining_part->Start()};
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

LayoutObjectAndOffset TraversePaintRange::ComputePaintRangeResult::MarkStart(
                                LayoutObject* start_layout_object,
                                WTF::Optional<unsigned> start_offset) {
  if (!start_layout_object->GetNode()->IsTextNode()) {
    DCHECK(!start_offset.has_value());
    MarkSelected(start_layout_object, SelectionState::kStart);
    return LayoutObjectAndOffset(start_layout_object);
  }

  DCHECK(start_offset.has_value());
  const unsigned unsigned_offset = start_offset.value();
  LayoutText* const start_layout_text = ToLayoutText(start_layout_object);
  if (unsigned_offset >= start_layout_text->TextStartOffset()) {
    // |start_offset| is within |start_layout_object| whether it has first
    // letter part or not.
    MarkSelected(start_layout_object, SelectionState::kStart);
    return {start_layout_text,
            unsigned_offset - start_layout_text->TextStartOffset()};
  }

  // |start_layout_object| has first letter part and |start_offset| is within
  // the part.
  LayoutTextFragment* const first_letter_part =
      FirstLetterPartFor(start_layout_object);
  DCHECK(first_letter_part);
  MarkSelected(first_letter_part, SelectionState::kStart);
  MarkSelected(start_layout_text, SelectionState::kInside);
  return {first_letter_part, start_offset.value()};
}

LayoutObjectAndOffset TraversePaintRange::ComputePaintRangeResult::MarkEnd(
                              LayoutObject* end_layout_object,
                              WTF::Optional<unsigned> end_offset) {
  if (!end_layout_object->GetNode()->IsTextNode()) {
    DCHECK(!end_offset.has_value());
    MarkSelected(end_layout_object, SelectionState::kEnd);
    return LayoutObjectAndOffset(end_layout_object);
  }

  DCHECK(end_offset.has_value());
  const unsigned unsigned_offset = end_offset.value();
  LayoutText* const end_layout_text = ToLayoutText(end_layout_object);
  if (unsigned_offset >= end_layout_text->TextStartOffset()) {
    // |end_offset| is within |end_layout_object| whether it has first
    // letter part or not.
    MarkSelected(end_layout_object, SelectionState::kEnd);
    if (LayoutTextFragment* const first_letter_part =
            FirstLetterPartFor(end_layout_object)) {
      MarkSelected(first_letter_part, SelectionState::kInside);
    }
    return {end_layout_text,
            unsigned_offset - end_layout_text->TextStartOffset()};
  }

  // |end_layout_object| has first letter part and |end_offset| is within
  // the part.
  LayoutTextFragment* const first_letter_part =
      FirstLetterPartFor(end_layout_object);
  DCHECK(first_letter_part);
  MarkSelected(first_letter_part, SelectionState::kEnd);
  return {first_letter_part, end_offset.value()};
}

void TraversePaintRange::ComputePaintRangeResult::MarkStartAndEndInTwoNodes(
                                      LayoutObject* start_layout_object,
                                      WTF::Optional<unsigned> start_offset,
                                      LayoutObject* end_layout_object,
                                      WTF::Optional<unsigned> end_offset) {
  const LayoutObjectAndOffset& start =
      MarkStart(start_layout_object, start_offset);
  const LayoutObjectAndOffset& end =
      MarkEnd(end_layout_object, end_offset);
  paint_range = { start.layout_object, start.offset, end.layout_object, end.offset };
}

static WTF::Optional<unsigned> GetTextContentOffset(
    LayoutObject* layout_object,
    WTF::Optional<unsigned> node_offset) {
  DCHECK(layout_object->EnclosingNGBlockFlow());
  // |layout_object| is start or end of selection and offset is only valid
  // if it is LayoutText.
  if (!layout_object->IsText())
    return WTF::nullopt;
  // There are LayoutText that selection can't be inside it(BR, WBR,
  // LayoutCounter).
  if (!node_offset.has_value())
    return WTF::nullopt;
  const Position position_in_dom(*layout_object->GetNode(),
                                 node_offset.value());
  const NGOffsetMapping* const offset_mapping =
      NGOffsetMapping::GetFor(position_in_dom);
  DCHECK(offset_mapping);
  const WTF::Optional<unsigned>& ng_offset =
      offset_mapping->GetTextContentOffset(position_in_dom);
  return ng_offset;
}

void TraversePaintRange::ComputePaintRangeResult::ComputeNewPaintRange(LayoutObject* start_layout_object,
                                 WTF::Optional<unsigned> start_node_offset,
                                 LayoutObject* end_layout_object,
                                 WTF::Optional<unsigned> end_node_offset) {
  if (paint_range.IsNull())
    return;
  LayoutObject* const start = paint_range.start_layout_object;
  // If LayoutObject is not in NG, use legacy offset.
  const WTF::Optional<unsigned> start_offset =
      start->EnclosingNGBlockFlow()
          ? GetTextContentOffset(start_layout_object, start_node_offset)
          : paint_range.start_offset;

  LayoutObject* const end = paint_range.end_layout_object;
  const WTF::Optional<unsigned> end_offset =
      end->EnclosingNGBlockFlow()
          ? GetTextContentOffset(end_layout_object, end_node_offset)
          : paint_range.end_offset;

  paint_range = {start, start_offset, end, end_offset};
}

// ClampOffset modifies |offset| fixed in a range of |text_fragment| start/end
// offsets.
static unsigned ClampOffset(unsigned offset,
                            const NGPhysicalTextFragment& text_fragment) {
  return std::min(std::max(offset, text_fragment.StartOffset()),
                  text_fragment.EndOffset());
}

std::pair<unsigned, unsigned> LayoutSelection::SelectionStartEndForNG(
    const NGPhysicalTextFragment& text_fragment) {
  // FrameSelection holds selection offsets in layout block flow at
  // LayoutSelection::Commit() if selection starts/ends within Text that
  // each LayoutObject::SelectionState indicates.
  // These offset can out of |text_fragment| because SelectionState is of each
  // LayoutText and not |text_fragment|.
  switch (text_fragment.GetLayoutObject()->GetSelectionState()) {
    case SelectionState::kStart: {
      DCHECK(SelectionStart().has_value());
      unsigned start_in_block = SelectionStart().value_or(0);
      return {ClampOffset(start_in_block, text_fragment),
              text_fragment.EndOffset()};
    }
    case SelectionState::kEnd: {
      DCHECK(SelectionEnd().has_value());
      unsigned end_in_block =
          SelectionEnd().value_or(text_fragment.EndOffset());
      return {text_fragment.StartOffset(),
              ClampOffset(end_in_block, text_fragment)};
    }
    case SelectionState::kStartAndEnd: {
      DCHECK(SelectionStart().has_value());
      DCHECK(SelectionEnd().has_value());
      unsigned start_in_block = SelectionStart().value_or(0);
      unsigned end_in_block =
          SelectionEnd().value_or(text_fragment.EndOffset());
      return {ClampOffset(start_in_block, text_fragment),
              ClampOffset(end_in_block, text_fragment)};
    }
    case SelectionState::kInside:
      return {text_fragment.StartOffset(), text_fragment.EndOffset()};
    default:
      // This block is not included in selection.
      return {0, 0};
  }
}

ComputePaintRangeResult TraversePaintRange::ComputePaintRange(
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

  if (!RuntimeEnabledFeatures::LayoutNGPaintFragmentsEnabled())
    return result;
  ComputeNewPaintRange(result, start_layout_object, start_offset,
                       end_layout_object, end_offset);
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
  if ((paint_range.start_layout_object->GetSelectionState() !=
           SelectionState::kStart &&
       paint_range.start_layout_object->GetSelectionState() !=
           SelectionState::kStartAndEnd) ||
      (paint_range.end_layout_object->GetSelectionState() !=
           SelectionState::kEnd &&
       paint_range.end_layout_object->GetSelectionState() !=
           SelectionState::kStartAndEnd)) {
    if (paint_range.start_layout_object == paint_range.end_layout_object) {
      paint_range.start_layout_object->LayoutObject::SetSelectionState(
          SelectionState::kStartAndEnd);
    } else {
      paint_range.start_layout_object->LayoutObject::SetSelectionState(
          SelectionState::kStart);
      paint_range.end_layout_object->LayoutObject::SetSelectionState(
          SelectionState::kEnd);
    }
  }
  // TODO(yoichio): If start == end, they should be kStartAndEnd.
  // If not, start.SelectionState == kStart and vice versa.
  DCHECK(paint_range.start_layout_object->GetSelectionState() ==
             SelectionState::kStart ||
         paint_range.start_layout_object->GetSelectionState() ==
             SelectionState::kStartAndEnd);
  DCHECK(paint_range.end_layout_object->GetSelectionState() ==
             SelectionState::kEnd ||
         paint_range.end_layout_object->GetSelectionState() ==
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

  paint_range_ =
      result.paint_range.IsNull() ? SelectionInFlatTree() : new_selection;
  start_offset_ = result.paint_range.start_offset;
  end_offset_ = result.paint_range.end_offset;
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
