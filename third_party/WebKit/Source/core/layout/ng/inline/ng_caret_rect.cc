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

// TODO(xiaochengh): Explain the resolution process.
enum class ResolutionType { kFailed, kFoundCandidate, kResolved };
struct CaretPositionResolution {
  ResolutionType type = ResolutionType::kFailed;
  CaretPosition caret_position;
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
  if (!last_line || &fragment != current_line.Children().front().get() ||
      IsLineBreakFragment(*last_line->Children().back()))
    return true;
  return false;
}

bool CanResolveCaretPositionAfterFragment(
    const NGPhysicalFragment& fragment,
    const NGPhysicalLineBoxFragment& current_line,
    TextAffinity affinity) {
  // There is not caret position after a line break fragment.
  if (IsLineBreakFragment(fragment))
    return false;
  // Text offset at fragment end can be resolved if we are looking for a
  // upstream caret position
  if (affinity == TextAffinity::kUpstream)
    return true;
  // Or if the fragment doesn't have continuation in the next line
  // TODO(xiaochengh): Should also check the next line box
  if (&fragment != current_line.Children().back().get())
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
    unsigned offset,
    TextAffinity affinity) {
  if (IsAnonymousTextFragment(fragment))
    return CaretPositionResolution();
  if (offset < fragment.StartOffset() || offset > fragment.EndOffset()) {
    // TODO(xiaochengh): This may introduce false negatives. Investigate.
    return CaretPositionResolution();
  }
  CaretPosition candidate = {
      &fragment, {}, CaretPositionType::kAtTextOffset, offset};

  // Offsets in the interior of a fragment can be resolved directly.
  if (offset > fragment.StartOffset() && offset < fragment.EndOffset())
    return {ResolutionType::kResolved, candidate};

  if (offset == fragment.StartOffset()) {
    if (CanResolveCaretPositionBeforeFragment(fragment, current_line, last_line,
                                              affinity))
      return {ResolutionType::kResolved, candidate};
  }

  if (offset == fragment.EndOffset()) {
    if (CanResolveCaretPositionAfterFragment(fragment, current_line, affinity))
      return {ResolutionType::kResolved, candidate};
  }

  // We may have a better candidate
  return {ResolutionType::kFoundCandidate, candidate};
}

CaretPositionResolution TryResolveCaretPositionByBoxFragmentSide(
    const NGPhysicalBoxFragment& fragment,
    const NGPhysicalLineBoxFragment& current_line,
    const NGPhysicalLineBoxFragment* last_line,
    unsigned offset,
    TextAffinity affinity) {
  // TODO(xiaochengh): Design more straightforward way to get text offset of
  // atomic inline box.
  const Node* node = fragment.GetNode();
  DCHECK(node);  // We shouldn't have anonymous atomic inline box.
  Optional<unsigned> maybe_offset_before =
      NGOffsetMapping::GetFor(Position::BeforeNode(*node))
          ->GetTextContentOffset(Position::BeforeNode(*node));
  // We should have offset mapping for atomic inline boxes.
  DCHECK(maybe_offset_before.has_value());
  const unsigned offset_before = maybe_offset_before.value();
  const unsigned offset_after = offset_before + 1;
  if (offset != offset_before && offset != offset_after)
    return CaretPositionResolution();
  CaretPositionType position_type = offset == offset_before
                                        ? CaretPositionType::kBeforeBox
                                        : CaretPositionType::kAfterBox;
  CaretPosition candidate = {&fragment, {}, position_type, WTF::nullopt};

  if (offset == offset_before) {
    if (CanResolveCaretPositionBeforeFragment(fragment, current_line, last_line,
                                              affinity))
      return {ResolutionType::kResolved, candidate};
  }

  if (offset == offset_after) {
    if (CanResolveCaretPositionAfterFragment(fragment, current_line, affinity))
      return {ResolutionType::kResolved, candidate};
  }

  return {ResolutionType::kFoundCandidate, candidate};
}

CaretPositionResolution TryResolveCaretPositionInFragment(
    const NGPhysicalFragmentWithOffset& fragment_and_offset,
    const NGPhysicalLineBoxFragment& current_line,
    const NGPhysicalLineBoxFragment* last_line,
    unsigned text_offset,
    TextAffinity affinity) {
  const NGPhysicalFragment& fragment = *fragment_and_offset.fragment;
  CaretPositionResolution resolution;
  if (fragment.IsText()) {
    resolution = TryResolveCaretPositionInTextFragment(
        ToNGPhysicalTextFragment(fragment), current_line, last_line,
        text_offset, affinity);
  } else if (fragment.IsBox() && fragment.IsInlineBlock()) {
    resolution = TryResolveCaretPositionByBoxFragmentSide(
        ToNGPhysicalBoxFragment(fragment), current_line, last_line, text_offset,
        affinity);
  }

  // TODO(xiaochengh): Handle caret poisition in empty container (e.g. empty
  // line box).

  resolution.caret_position.offset_to_container_box =
      fragment_and_offset.offset_to_container_box;
  return resolution;
}

NGPhysicalOffsetRect ComputeLocalCaretRectByBoxSide(
    const NGPhysicalBoxFragment&,
    CaretPositionType) {
  // TODO(xiaochengh): Implementation.
  return {};
}

NGPhysicalOffsetRect ComputeLocalCaretRectAtTextOffset(
    const NGPhysicalTextFragment&,
    unsigned) {
  // TODO(xiaochengh): Implementation.
  return {};
}

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

CaretPosition ComputeNGCaretPosition(const LayoutBlockFlow& context,
                                     unsigned offset,
                                     TextAffinity affinity) {
  const NGPhysicalBoxFragment* box_fragment = context.CurrentFragment();
  // TODO(kojii): CurrentFragment isn't always available after layout clean.
  // Investigate why.
  if (!box_fragment)
    return CaretPosition();
  Vector<CaretPosition> candidates;
  const NGPhysicalLineBoxFragment* last_line;
  const NGPhysicalLineBoxFragment* current_line;
  for (const auto& child :
       NGInlineFragmentTraversal::DescendantsOf(*box_fragment)) {
    if (child.fragment->IsLineBox()) {
      last_line = current_line;
      current_line = ToNGPhysicalLineBoxFragment(child.fragment);
    }

    DCHECK(current_line);
    CaretPositionResolution resolution = TryResolveCaretPositionInFragment(
        child, *current_line, last_line, offset, affinity);
    if (resolution.type == ResolutionType::kResolved)
      return resolution.caret_position;
    if (resolution.type == ResolutionType::kFoundCandidate)
      candidates.push_back(resolution.caret_position);
  }

  if (candidates.IsEmpty())
    return CaretPosition();

  // TODO(xiaochengh): Choose the correct candidate.
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

  CaretPosition caret_position =
      ComputeNGCaretPosition(context, offset, affinity);
  if (!caret_position.fragment)
    return LocalCaretRect();

  NGPhysicalOffsetRect fragment_local_rect;
  switch (caret_position.position_type) {
    case CaretPositionType::kBeforeBox:
    case CaretPositionType::kAfterBox:
      DCHECK(caret_position.fragment->IsBox());
      fragment_local_rect = ComputeLocalCaretRectByBoxSide(
          ToNGPhysicalBoxFragment(*caret_position.fragment),
          caret_position.position_type);
      break;
    case CaretPositionType::kAtTextOffset:
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
