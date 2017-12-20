// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/layout/ng/inline/ng_caret_rect.h"

#include "core/editing/PositionWithAffinity.h"
#include "core/editing/VisibleUnits.h"
#include "core/layout/LayoutBlockFlow.h"
#include "core/layout/LayoutTextFragment.h"
#include "core/layout/ng/geometry/ng_physical_offset_rect.h"
#include "core/layout/ng/inline/ng_inline_fragment_iterator.h"
#include "core/layout/ng/inline/ng_offset_mapping.h"
#include "core/layout/ng/inline/ng_physical_line_box_fragment.h"
#include "core/layout/ng/inline/ng_physical_text_fragment.h"
#include "core/layout/ng/ng_physical_box_fragment.h"

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

bool IsLineBreakFragment(const NGPhysicalFragment& fragment) {
  if (!fragment.IsText())
    return false;
  // TODO(xiaochengh): Find a less hacky way to do so
  return ToNGPhysicalTextFragment(fragment).Text() == "\n";
}

bool CanResolveCaretPositionBeforeFragment(
    const NGPhysicalFragment& fragment,
    const NGPhysicalLineBoxFragment& current_line,
    const NGPhysicalLineBoxFragment* last_line,
    TextAffinity affinity) {
  // Text offset at fragment start can be resolved if we are looking for a
  // downstream caret position
  if (affinity == TextAffinity::kDownstream)
    return true;
  // Or if the fragment is not the continuation of the last fragment of the
  // last line
  // TODO(xiaochengh): Verify if this works for RtL
  if (!last_line || &fragment != current_line.Children().front().get() ||
      IsLineBreakFragment(*last_line->Children().back()))
    return true;
  return false;
}

bool CanResolveCaretPositionAfterFragment(
    const NGPhysicalFragment& fragment,
    const NGPhysicalLineBoxFragment& current_line,
    const NGPhysicalLineBoxFragment* next_line,
    TextAffinity affinity) {
  // There is not caret position after a line break fragment.
  if (IsLineBreakFragment(fragment))
    return false;
  // Text offset at fragment end can be resolved if we are looking for a
  // upstream caret position
  if (affinity == TextAffinity::kUpstream)
    return true;
  // Or if the fragment doesn't have continuation in the next line
  // TODO(xiaochengh): Verify if this works for RtL
  if (!next_line || &fragment != current_line.Children().back().get())
    return true;
  return false;
}

bool IsAnonymousTextFragment(const NGPhysicalTextFragment& fragment) {
  const LayoutObject* layout_object = fragment.GetLayoutObject();
  DCHECK(layout_object->IsText());
  if (ToLayoutText(layout_object)->IsTextFragment())
    return !ToLayoutTextFragment(layout_object)->AssociatedTextNode();
  return !layout_object->GetNode();
}

CaretPositionResolution TryResolveCaretPositionInTextFragment(
    const NGPhysicalTextFragment& fragment,
    const NGPhysicalLineBoxFragment& current_line,
    const NGPhysicalLineBoxFragment* last_line,
    const NGPhysicalLineBoxFragment* next_line,
    unsigned offset,
    TextAffinity affinity) {
  if (IsAnonymousTextFragment(fragment))
    return CaretPositionResolution();
  if (offset < fragment.StartOffset() || offset > fragment.EndOffset()) {
    // TODO(xiaochengh): This may introduce false negatives. Investigate.
    return CaretPositionResolution();
  }
  NGCaretPosition candidate = {
      &fragment, {}, NGCaretPositionType::kAtTextOffset, offset};

  // Offsets in the interior of a fragment can be resolved directly.
  if (offset > fragment.StartOffset() && offset < fragment.EndOffset())
    return {ResolutionType::kResolved, candidate};

  if (offset == fragment.StartOffset()) {
    if (CanResolveCaretPositionBeforeFragment(fragment, current_line, last_line,
                                              affinity))
      return {ResolutionType::kResolved, candidate};
  }

  if (offset == fragment.EndOffset()) {
    if (CanResolveCaretPositionAfterFragment(fragment, current_line, next_line,
                                             affinity))
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
    const NGPhysicalLineBoxFragment* next_line,
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
  NGCaretPositionType position_type = offset == offset_before
                                          ? NGCaretPositionType::kBeforeBox
                                          : NGCaretPositionType::kAfterBox;
  NGCaretPosition candidate = {&fragment, {}, position_type, WTF::nullopt};

  if (offset == offset_before) {
    if (CanResolveCaretPositionBeforeFragment(fragment, current_line, last_line,
                                              affinity))
      return {ResolutionType::kResolved, candidate};
  }

  if (offset == offset_after) {
    if (CanResolveCaretPositionAfterFragment(fragment, current_line, next_line,
                                             affinity))
      return {ResolutionType::kResolved, candidate};
  }

  return {ResolutionType::kFoundCandidate, candidate};
}

// -------------------------------------

// Helpers for converting NGCaretPositions to caret rects.

NGPhysicalOffsetRect ComputeLocalCaretRectByBoxSide(
    const NGPhysicalBoxFragment&,
    NGCaretPositionType) {
  // TODO(xiaochengh): Implementation.
  return {};
}

NGPhysicalOffsetRect ComputeLocalCaretRectAtTextOffset(
    const NGPhysicalTextFragment&,
    unsigned) {
  // TODO(xiaochengh): Implementation.
  return {};
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
    const NGPhysicalLineBoxFragment* line_box;
    Vector<NGPhysicalFragmentWithOffset> fragments;
  };

  Vector<LineBoxAndFragments> lines;
  for (const auto& child :
       NGInlineFragmentTraversal::DescendantsOf(*box_fragment)) {
    if (child.fragment->IsLineBox())
      lines.push_back(
          LineBoxAndFragments{ToNGPhysicalLineBoxFragment(child.fragment), {}});

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

  Vector<NGCaretPosition> candidates;
  for (const auto& line : lines) {
    const NGPhysicalLineBoxFragment& current_line = *line.line_box;
    const NGPhysicalLineBoxFragment* last_line =
        (&line != lines.begin() ? std::prev(&line)->line_box : nullptr);
    const NGPhysicalLineBoxFragment* next_line =
        (std::next(&line) != lines.end() ? std::next(&line)->line_box
                                         : nullptr);

    for (const auto& child : line.fragments) {
      CaretPositionResolution resolution;
      const NGPhysicalFragment& fragment = *child.fragment;
      if (fragment.IsText()) {
        resolution = TryResolveCaretPositionInTextFragment(
            ToNGPhysicalTextFragment(fragment), current_line, last_line,
            next_line, offset, affinity);
      } else if (fragment.IsBox() && fragment.IsInlineBlock()) {
        resolution = TryResolveCaretPositionByBoxFragmentSide(
            ToNGPhysicalBoxFragment(fragment), current_line, last_line,
            next_line, offset, affinity);
      }

      // TODO(xiaochengh): Handle caret poisition in empty container (e.g. empty
      // line box).

      resolution.caret_position.offset_to_container_box =
          child.offset_to_container_box;
      if (resolution.type == ResolutionType::kResolved)
        return resolution.caret_position;
      if (resolution.type == ResolutionType::kFoundCandidate)
        candidates.push_back(resolution.caret_position);
    }
  }

  if (candidates.IsEmpty())
    return NGCaretPosition();

  // TODO(xiaochengh): Choose the correct candidate from multiple ones.
  return candidates.front();
}

LocalCaretRect ComputeNGLocalCaretRect(const LayoutBlockFlow& context,
                                       const PositionWithAffinity& position) {
  AssertValidPositionForCaretRectComputation(position);
  DCHECK_EQ(&context, NGInlineFormattingContextOf(position.GetPosition()));
  DCHECK(NGOffsetMapping::GetFor(&context));
  const NGOffsetMapping& mapping = *NGOffsetMapping::GetFor(&context);
  Optional<unsigned> maybe_offset =
      mapping.GetTextContentOffset(position.GetPosition());
  if (!maybe_offset.has_value()) {
    // TODO(xiaochengh): Investigate if we reach here.
    NOTREACHED();
    return LocalCaretRect();
  }

  const unsigned offset = maybe_offset.value();
  const TextAffinity affinity = position.Affinity();

  NGCaretPosition caret_position =
      ComputeNGCaretPosition(context, offset, affinity);
  if (!caret_position.fragment)
    return LocalCaretRect();

  NGPhysicalOffsetRect fragment_local_rect;
  switch (caret_position.position_type) {
    case NGCaretPositionType::kBeforeBox:
    case NGCaretPositionType::kAfterBox:
      DCHECK(caret_position.fragment->IsBox());
      fragment_local_rect = ComputeLocalCaretRectByBoxSide(
          ToNGPhysicalBoxFragment(*caret_position.fragment),
          caret_position.position_type);
      break;
    case NGCaretPositionType::kAtTextOffset:
      DCHECK(caret_position.fragment->IsText());
      DCHECK(caret_position.text_offset.has_value());
      fragment_local_rect = ComputeLocalCaretRectAtTextOffset(
          ToNGPhysicalTextFragment(*caret_position.fragment),
          *caret_position.text_offset);
      break;
    default:
      NOTREACHED();
  }

  NGPhysicalOffsetRect context_local_rect =
      fragment_local_rect + caret_position.offset_to_container_box;
  return {caret_position.fragment->GetLayoutObject(),
          context_local_rect.ToLayoutRect()};
}

LocalCaretRect ComputeNGLocalCaretRect(
    const LayoutBlockFlow& context,
    const PositionInFlatTreeWithAffinity& position) {
  const PositionWithAffinity dom_position(
      ToPositionInDOMTree(position.GetPosition()), position.Affinity());
  return ComputeNGLocalCaretRect(context, dom_position);
}

}  // namespace blink
