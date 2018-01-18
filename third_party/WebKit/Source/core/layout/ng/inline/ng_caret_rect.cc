// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/layout/ng/inline/ng_caret_rect.h"

#include "core/editing/LocalCaretRect.h"
#include "core/editing/PositionWithAffinity.h"
#include "core/layout/LayoutBlockFlow.h"
#include "core/layout/LayoutTextFragment.h"
#include "core/layout/ng/geometry/ng_physical_offset_rect.h"
#include "core/layout/ng/inline/ng_inline_break_token.h"
#include "core/layout/ng/inline/ng_inline_fragment_traversal.h"
#include "core/layout/ng/inline/ng_offset_mapping.h"
#include "core/layout/ng/inline/ng_physical_line_box_fragment.h"
#include "core/layout/ng/inline/ng_physical_text_fragment.h"
#include "core/layout/ng/ng_physical_box_fragment.h"
#include "platform/fonts/CharacterRange.h"

namespace blink {

namespace {

// -------------------------------------

// Helpers for caret position resolution. See the "caret resolution
// process" in ComputeNGCaretPosition() for documentation.

enum class ResolutionType { kFailed, kFoundCandidate, kResolved };
struct CaretPositionResolution {
  ResolutionType type = ResolutionType::kFailed;
  NGCaretPosition caret_position;
};

// TODO(xiaochengh): Find a clean way to move this function as
// a member of NGPhysicalFragment.
bool IsFragmentContinuationFromLastLine(
    const NGPhysicalFragment& fragment,
    const NGPhysicalLineBoxFragment& current_line,
    const NGPhysicalLineBoxFragment* last_line) {
  if (!last_line)
    return false;
  // Only the logically first fragment in each line can be continuation from the
  // last line.
  // TODO(xiaochengh): Verify if this works for RtL
  if (&fragment != current_line.Children().front().get())
    return false;
  DCHECK(last_line->BreakToken());
  DCHECK(last_line->BreakToken()->IsInlineType());
  DCHECK(!last_line->BreakToken()->IsFinished());
  return !ToNGInlineBreakToken(last_line->BreakToken())->IsForcedBreak();
}

bool CanResolveCaretPositionBeforeFragment(
    const NGPhysicalFragment& fragment,
    TextAffinity affinity,
    const NGPhysicalLineBoxFragment& current_line,
    const NGPhysicalLineBoxFragment* last_line) {
  if (affinity == TextAffinity::kDownstream)
    return true;
  return !IsFragmentContinuationFromLastLine(fragment, current_line, last_line);
}

// TODO(xiaochengh): Find a clean way to move this function as
// a member of NGPhysicalFragment.
bool FragmentContinuesInNextLine(
    const NGPhysicalFragment& fragment,
    const NGPhysicalLineBoxFragment& current_line) {
  // Only the logically last fragment in each line can have continuation in the
  // next line.
  // TODO(xiaochengh): Verify if this works for RtL
  if (&fragment != current_line.Children().back().get())
    return false;
  DCHECK(current_line.BreakToken());
  DCHECK(current_line.BreakToken()->IsInlineType());
  const NGInlineBreakToken& break_token =
      ToNGInlineBreakToken(*current_line.BreakToken());
  return !break_token.IsFinished() && !break_token.IsForcedBreak();
}

bool CanResolveCaretPositionAfterFragment(
    const NGPhysicalFragment& fragment,
    TextAffinity affinity,
    const NGPhysicalLineBoxFragment& current_line) {
  if (affinity == TextAffinity::kUpstream)
    return true;
  return !FragmentContinuesInNextLine(fragment, current_line);
}

CaretPositionResolution TryResolveCaretPositionInTextFragment(
    const NGPhysicalTextFragment& fragment,
    const NGPhysicalLineBoxFragment& current_line,
    const NGPhysicalLineBoxFragment* last_line,
    unsigned offset,
    TextAffinity affinity) {
  if (fragment.IsAnonymousText())
    return CaretPositionResolution();

  // [StartOffset(), EndOffset()] is the range allowing caret placement.
  // For example, "foo" has 4 offsets allowing caret placement.
  if (offset < fragment.StartOffset() || offset > fragment.EndOffset()) {
    // TODO(xiaochengh): This may introduce false negatives. Investigate.
    return CaretPositionResolution();
  }
  NGCaretPosition candidate = {&fragment, NGCaretPositionType::kAtTextOffset,
                               offset};

  // Offsets in the interior of a fragment can be resolved directly.
  if (offset > fragment.StartOffset() && offset < fragment.EndOffset())
    return {ResolutionType::kResolved, candidate};

  if (offset == fragment.StartOffset() &&
      CanResolveCaretPositionBeforeFragment(fragment, affinity, current_line,
                                            last_line)) {
    return {ResolutionType::kResolved, candidate};
  }

  if (offset == fragment.EndOffset() && !fragment.IsLineBreak() &&
      CanResolveCaretPositionAfterFragment(fragment, affinity, current_line)) {
    return {ResolutionType::kResolved, candidate};
  }

  // We may have a better candidate
  return {ResolutionType::kFoundCandidate, candidate};
}

unsigned GetTextOffsetBefore(const NGPhysicalBoxFragment& fragment) {
  // TODO(xiaochengh): Design more straightforward way to get text offset of
  // atomic inline box.
  DCHECK(fragment.IsInlineBlock());
  const Node* node = fragment.GetNode();
  DCHECK(node);
  const Position before_node = Position::BeforeNode(*node);
  Optional<unsigned> maybe_offset_before =
      NGOffsetMapping::GetFor(before_node)->GetTextContentOffset(before_node);
  // We should have offset mapping for atomic inline boxes.
  DCHECK(maybe_offset_before.has_value());
  return maybe_offset_before.value();
}

CaretPositionResolution TryResolveCaretPositionByBoxFragmentSide(
    const NGPhysicalBoxFragment& fragment,
    const NGPhysicalLineBoxFragment& current_line,
    const NGPhysicalLineBoxFragment* last_line,
    unsigned offset,
    TextAffinity affinity) {
  if (!fragment.GetNode()) {
    // TODO(xiaochengh): This leads to false negatives for, e.g., RUBY, where an
    // anonymous wrapping inline block is created.
    return CaretPositionResolution();
  }

  const unsigned offset_before = GetTextOffsetBefore(fragment);
  const unsigned offset_after = offset_before + 1;
  if (offset != offset_before && offset != offset_after)
    return CaretPositionResolution();
  const NGCaretPositionType position_type =
      offset == offset_before ? NGCaretPositionType::kBeforeBox
                              : NGCaretPositionType::kAfterBox;
  NGCaretPosition candidate{&fragment, position_type, WTF::nullopt};

  if (offset == offset_before &&
      CanResolveCaretPositionBeforeFragment(fragment, affinity, current_line,
                                            last_line)) {
    return {ResolutionType::kResolved, candidate};
  }

  if (offset == offset_after &&
      CanResolveCaretPositionAfterFragment(fragment, affinity, current_line)) {
    return {ResolutionType::kResolved, candidate};
  }

  return {ResolutionType::kFoundCandidate, candidate};
}

CaretPositionResolution TryResolveCaretPositionWithFragment(
    const NGPhysicalFragment& fragment,
    const NGPhysicalLineBoxFragment& current_line,
    const NGPhysicalLineBoxFragment* last_line,
    unsigned offset,
    TextAffinity affinity) {
  if (fragment.IsText()) {
    return TryResolveCaretPositionInTextFragment(
        ToNGPhysicalTextFragment(fragment), current_line, last_line, offset,
        affinity);
  }
  if (fragment.IsBox() && fragment.IsInlineBlock()) {
    return TryResolveCaretPositionByBoxFragmentSide(
        ToNGPhysicalBoxFragment(fragment), current_line, last_line, offset,
        affinity);
  }
  return CaretPositionResolution();
}

// -------------------------------------

// Helpers for converting NGCaretPositions to caret rects.

std::pair<const NGPhysicalLineBoxFragment&, NGPhysicalOffset>
GetLineBoxAndOffset(const NGPhysicalBoxFragment& fragment) {
  DCHECK(fragment.GetNode());
  const LayoutBlockFlow* context =
      NGInlineFormattingContextOf(Position::BeforeNode(*fragment.GetNode()));
  DCHECK(context);
  DCHECK(context->CurrentFragment());

  NGPhysicalOffset self_offset_to_container_box;
  NGPhysicalOffset line_box_offset_to_container_box;
  const NGPhysicalLineBoxFragment* line_box = nullptr;
  for (const auto& ancestor : NGInlineFragmentTraversal::InclusiveAncestorsOf(
           *context->CurrentFragment(), fragment)) {
    if (ancestor.fragment == &fragment) {
      self_offset_to_container_box = ancestor.offset_to_container_box;
      continue;
    }

    if (ancestor.fragment->IsLineBox()) {
      line_box = ToNGPhysicalLineBoxFragment(ancestor.fragment.get());
      line_box_offset_to_container_box = ancestor.offset_to_container_box;
      break;
    }
  }

  DCHECK(line_box);
  return {*line_box,
          self_offset_to_container_box - line_box_offset_to_container_box};
}

NGPhysicalOffsetRect ComputeLocalCaretRectByBoxSide(
    const NGPhysicalBoxFragment& fragment,
    NGCaretPositionType position_type) {
  const LocalFrameView* frame_view =
      fragment.GetLayoutObject()->GetDocument().View();
  const LayoutUnit caret_width = frame_view->CaretWidth();

  // TODO(xiaochengh): Handle vertical writing mode.

  const bool is_ltr = fragment.Style().Direction() == TextDirection::kLtr;
  const bool is_left_side =
      (position_type == NGCaretPositionType::kBeforeBox) == is_ltr;
  const LayoutUnit caret_left =
      is_left_side ? LayoutUnit() : fragment.Size().width - caret_width;

  std::pair<const NGPhysicalLineBoxFragment&, NGPhysicalOffset>
      line_box_and_offset = GetLineBoxAndOffset(fragment);
  const LayoutUnit caret_height = line_box_and_offset.first.Size().height;
  const LayoutUnit caret_top = -line_box_and_offset.second.top;

  const NGPhysicalOffset caret_location(caret_left, caret_top);
  const NGPhysicalSize caret_size(caret_width, caret_height);
  return NGPhysicalOffsetRect(caret_location, caret_size);
}

NGPhysicalOffsetRect ComputeLocalCaretRectAtTextOffset(
    const NGPhysicalTextFragment& fragment,
    unsigned offset) {
  const LocalFrameView* frame_view =
      fragment.GetLayoutObject()->GetDocument().View();
  LayoutUnit caret_width = frame_view->CaretWidth();

  const bool is_horizontal = fragment.Style().IsHorizontalWritingMode();

  LayoutUnit caret_height =
      is_horizontal ? fragment.Size().height : fragment.Size().width;
  LayoutUnit caret_top;

  // TODO(xiaochengh): Add unit tests about LocalCaretRect at BR into
  // LocalCaretRectTest.

  LayoutUnit caret_left;
  if (!fragment.IsLineBreak()) {
    unsigned offset_in_fragment = offset - fragment.StartOffset();
    const ShapeResult* shape_result = fragment.TextShapeResult();
    DCHECK(shape_result);
    CharacterRange character_range =
        shape_result->GetCharacterRange(offset_in_fragment, offset_in_fragment);

    // Adjust the caret rect to ensure that it completely falls in the fragment.
    LayoutUnit caret_center = LayoutUnit(character_range.start);
    caret_left = caret_center - caret_width / 2;
    caret_left = std::max(LayoutUnit(), caret_left);
    caret_left = std::min(
        (is_horizontal ? fragment.Size().width : fragment.Size().height) -
            caret_width,
        caret_left);
    caret_left = LayoutUnit(caret_left.Round());
  }

  if (!is_horizontal) {
    std::swap(caret_top, caret_left);
    std::swap(caret_width, caret_height);
  }

  // Adjust the location to be relative to the inline formatting context.
  NGPhysicalOffset caret_location(caret_left, caret_top);
  for (const auto& child : NGInlineFragmentTraversal::DescendantsOf(
           *fragment.GetLayoutObject()->EnclosingBlockFlowFragment())) {
    if (child.fragment == &fragment) {
      caret_location += child.offset_to_container_box;
      break;
    }
  }

  NGPhysicalSize caret_size(caret_width, caret_height);
  return NGPhysicalOffsetRect(caret_location, caret_size);
}

LocalCaretRect ComputeLocalCaretRect(const NGCaretPosition& caret_position) {
  if (caret_position.IsNull())
    return LocalCaretRect();

  switch (caret_position.position_type) {
    case NGCaretPositionType::kBeforeBox:
    case NGCaretPositionType::kAfterBox: {
      DCHECK(caret_position.fragment->IsBox());
      const NGPhysicalOffsetRect fragment_local_rect =
          ComputeLocalCaretRectByBoxSide(
              ToNGPhysicalBoxFragment(*caret_position.fragment),
              caret_position.position_type);
      return {caret_position.fragment->GetLayoutObject(),
              fragment_local_rect.ToLayoutRect()};
    }
    case NGCaretPositionType::kAtTextOffset: {
      DCHECK(caret_position.fragment->IsText());
      DCHECK(caret_position.text_offset.has_value());
      const NGPhysicalOffsetRect caret_rect = ComputeLocalCaretRectAtTextOffset(
          ToNGPhysicalTextFragment(*caret_position.fragment),
          *caret_position.text_offset);

      // TODO(xiaochengh): This doesn't work for vertical-rl writing mode.
      return {caret_position.fragment->GetLayoutObject(),
              caret_rect.ToLayoutRect()};
    }
  }

  NOTREACHED();
  return {caret_position.fragment->GetLayoutObject(), LayoutRect()};
}

// -------------------------------------

void AssertValidPositionForCaretRectComputation(
    const PositionWithAffinity& position) {
#if DCHECK_IS_ON()
  DCHECK(NGOffsetMapping::AcceptsPosition(position.GetPosition()));
  const LayoutObject* layout_object = position.AnchorNode()->GetLayoutObject();
  DCHECK(layout_object);
  DCHECK(layout_object->IsText() || layout_object->IsAtomicInlineLevel());
#endif
}

}  // namespace

NGCaretPosition ComputeNGCaretPosition(const LayoutBlockFlow& context,
                                       unsigned offset,
                                       TextAffinity affinity) {
  const NGPhysicalBoxFragment* box_fragment = context.CurrentFragment();
  // TODO(kojii): CurrentFragment isn't always available after layout clean.
  // Investigate why.
  if (!box_fragment)
    return NGCaretPosition();

  // 1. Reorganize all inline fragments by lines
  // TODO(xiaochengh): We may want higher-level APIs for it.

  struct LineBoxAndFragments {
    LineBoxAndFragments(const NGPhysicalLineBoxFragment* line_box)
        : line_box(line_box) {}

    const NGPhysicalLineBoxFragment* line_box;
    Vector<NGPhysicalFragmentWithOffset> fragments;
  };

  Vector<LineBoxAndFragments> lines;
  for (const auto& child :
       NGInlineFragmentTraversal::DescendantsOf(*box_fragment)) {
    if (child.fragment->IsLineBox())
      lines.emplace_back(ToNGPhysicalLineBoxFragment(child.fragment.get()));

    DCHECK(lines.size());
    lines.back().fragments.push_back(child);
  }

  // 2. Re-iterate all inline fragments and try to resolve the caret position,
  // following the "caret resolution process":
  //
  // The (offset, affinity) pair is compared against each inline fragment to see
  // if the corresponding caret should be placed in the fragment, using the
  // |TryResolveCaretPositionInFoo()| functions. These functions may return:
  // - Failed, indicating that the caret must not be placed in the fragment;
  // - Resolved, indicating that the care should be placed in the fragment, and
  //   no further search is required. The result NGCaretPosition is returned
  //   together.
  // - FoundCandidate, indicating that the caret may be placed in the fragment;
  //   however, further search may find a better position. The candidate
  //   NGCaretPosition is also returned together.

  NGCaretPosition candidate;
  for (const auto& line : lines) {
    const NGPhysicalLineBoxFragment& current_line = *line.line_box;
    const NGPhysicalLineBoxFragment* last_line =
        (&line != lines.begin() ? std::prev(&line)->line_box : nullptr);

    for (const auto& child : line.fragments) {
      const CaretPositionResolution resolution =
          TryResolveCaretPositionWithFragment(*child.fragment, current_line,
                                              last_line, offset, affinity);

      if (resolution.type == ResolutionType::kFailed)
        continue;

      // TODO(xiaochengh): Handle caret poisition in empty container (e.g. empty
      // line box).

      if (resolution.type == ResolutionType::kResolved)
        return resolution.caret_position;

      DCHECK_EQ(ResolutionType::kFoundCandidate, resolution.type);
      // TODO(xiaochengh): We are not sure if we can ever find multiple
      // candidates. Handle it once reached.
      DCHECK(candidate.IsNull());
      candidate = resolution.caret_position;
    }
  }

  return candidate;
}

LocalCaretRect ComputeNGLocalCaretRect(const LayoutBlockFlow& context,
                                       const PositionWithAffinity& position) {
  AssertValidPositionForCaretRectComputation(position);
  DCHECK_EQ(&context, NGInlineFormattingContextOf(position.GetPosition()));
  DCHECK(NGOffsetMapping::GetFor(&context));
  const NGOffsetMapping& mapping = *NGOffsetMapping::GetFor(&context);
  const Optional<unsigned> maybe_offset =
      mapping.GetTextContentOffset(position.GetPosition());
  if (!maybe_offset.has_value()) {
    // TODO(xiaochengh): Investigate if we reach here.
    NOTREACHED();
    return LocalCaretRect();
  }

  const unsigned offset = maybe_offset.value();
  const TextAffinity affinity = position.Affinity();

  const NGCaretPosition caret_position =
      ComputeNGCaretPosition(context, offset, affinity);
  return ComputeLocalCaretRect(caret_position);
}

}  // namespace blink
