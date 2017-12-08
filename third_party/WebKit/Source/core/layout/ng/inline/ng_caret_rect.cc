// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/layout/ng/inline/ng_caret_rect.h"

#include "core/editing/PositionWithAffinity.h"
#include "core/editing/VisibleUnits.h"
#include "core/layout/LayoutBlockFlow.h"
#include "core/layout/ng/geometry/ng_physical_offset_rect.h"
#include "core/layout/ng/inline/ng_inline_fragment_iterator.h"
#include "core/layout/ng/inline/ng_offset_mapping.h"
#include "core/layout/ng/inline/ng_physical_text_fragment.h"
#include "core/layout/ng/ng_physical_box_fragment.h"

namespace blink {

namespace {

enum class CaretPositionType {
  kBeforeBox,
  kAfterBox,
  kAtTextOffset
};

struct CaretPosition {
  const NGPhysicalFragment* fragment = nullptr;
  NGPhysicalOffset offset_to_container_box;
  CaretPositionType position_type;
  Optional<unsigned> text_offset;
};

// TODO(xiaochengh): Explain the resolution process.
enum class ResolutionType { kFailed, kFoundCandidate, kResolved };
struct CaretPositionResolution {
  ResolutionType type = ResolutionType::kFailed;
  CaretPosition caret_position;
};

CaretPositionResolution TryResolveCaretPositionInTextFragment(
    const NGPhysicalTextFragment& fragment,
    unsigned offset,
    TextAffinity affinity) {
  if (offset < fragment.StartOffset() || offset > fragment.EndOffset()) {
    // TODO(xiaochengh): This may introduce false negatives. Investigate.
    return CaretPositionResolution();
  }
  const unsigned offset_in_fragment = offset - fragment.StartOffset();
  CaretPosition candidate = {
      &fragment, {}, CaretPositionType::kAtTextOffset, offset_in_fragment};
  if ((offset && offset < fragment.Length()) ||
      (!offset && affinity == TextAffinity::kDownstream) ||
      (offset == fragment.Length() && affinity == TextAffinity::kUpstream))
    return {ResolutionType::kResolved, candidate};
  return {ResolutionType::kFoundCandidate, candidate};
}

CaretPositionResolution TryResolveCaretPositionByBoxFragmentSide(
    const NGPhysicalBoxFragment& fragment,
    unsigned offset,
    TextAffinity affinity) {
  // TODO(xiaochengh): Design more straightforward way to get text offset of
  // atomic inline box.
  const Node* node = fragment.GetNode();
  if (!node) {
    // TODO(xiaochengh): Can we have anonymous atomic inline boxes?
    return CaretPositionResolution();
  }
  Optional<unsigned> maybe_offset_before =
      NGOffsetMapping::GetFor(Position::BeforeNode(*node))
          ->GetTextContentOffset(Position::BeforeNode(*node));
  if (!maybe_offset_before.has_value()) {
    // TODO(xiaochengh): This shouldn't be possible. Verify.
    return CaretPositionResolution();
  }
  const unsigned offset_before = maybe_offset_before.value();
  const unsigned offset_after = offset_before + 1;
  if (offset != offset_before && offset != offset_after)
    return CaretPositionResolution();
  CaretPositionType position_type = offset == offset_before
                                        ? CaretPositionType::kBeforeBox
                                        : CaretPositionType::kAfterBox;
  CaretPosition candidate = {&fragment, {}, position_type, WTF::nullopt};
  if ((offset == offset_before && affinity == TextAffinity::kDownstream) ||
      (offset == offset_after && affinity == TextAffinity::kUpstream))
    return {ResolutionType::kResolved, candidate};
  return {ResolutionType::kFoundCandidate, candidate};
}

CaretPositionResolution TryResolveCaretPositionInFragment(
    const NGPhysicalFragmentWithOffset& fragment_and_offset,
    unsigned text_offset,
    TextAffinity affinity) {
  const NGPhysicalFragment& fragment = *fragment_and_offset.fragment;
  CaretPositionResolution resolution;
  if (fragment.IsText()) {
    resolution = TryResolveCaretPositionInTextFragment(
        ToNGPhysicalTextFragment(fragment), text_offset, affinity);
  } else if (fragment.IsBox() && fragment.IsInlineBlock()) {
    // TODO(xiaochengh): Does this mean atomic inline?
    resolution = TryResolveCaretPositionByBoxFragmentSide(
        ToNGPhysicalBoxFragment(fragment), text_offset, affinity);
  }

  // TODO(xiaochengh): Handle caret poisition in empty container (e.g. empty
  // line box).

  resolution.caret_position.offset_to_container_box =
      fragment_and_offset.offset_to_container_box;
  return resolution;
}

CaretPosition ComputeCaretPosition(const LayoutBlockFlow& context,
                                   unsigned offset,
                                   TextAffinity affinity) {
  const NGPhysicalBoxFragment* box_fragment = context.CurrentFragment();
  // TODO(kojii): CurrentFragment isn't always available after layout clean.
  // Investigate why.
  if (!box_fragment)
    return CaretPosition();
  Vector<CaretPosition> candidates;
  for (const auto& child :
       NGInlineFragmentTraversal::DescendantsOf(*box_fragment)) {
    CaretPositionResolution resolution =
        TryResolveCaretPositionInFragment(child, offset, affinity);
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

LocalCaretRect ComputeNGLocalCaretRect(const LayoutBlockFlow& context,
                                       const PositionWithAffinity& position) {
  AssertValidPositionForCaretRectComputation(position);
  DCHECK_EQ(&context, NGInlineFormattingContextOf(position.GetPosition()));
  DCHECK(NGOffsetMapping::GetFor(&context));
  const NGOffsetMapping& mapping = *NGOffsetMapping::GetFor(&context);
  Optional<unsigned> maybe_offset =
      mapping.GetTextContentOffset(position.GetPosition());
  if (!maybe_offset.has_value()) {
    // TODO(xiaochengh): When do we reach here.
    return LocalCaretRect();
  }

  const unsigned offset = maybe_offset.value();
  const TextAffinity affinity = position.Affinity();

  CaretPosition caret_position =
      ComputeCaretPosition(context, offset, affinity);
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
