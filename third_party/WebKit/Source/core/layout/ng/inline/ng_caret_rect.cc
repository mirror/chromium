// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/layout/ng/inline/ng_caret_rect.h"

#include "core/editing/PositionWithAffinity.h"
#include "core/editing/VisibleUnits.h"
#include "core/layout/LayoutBlockFlow.h"
#include "core/layout/ng/geometry/ng_physical_offset_rect.h"
#include "core/layout/ng/inline/ng_offset_mapping.h"
#include "core/layout/ng/inline/ng_inline_fragment_iterator.h"
#include "core/layout/ng/inline/ng_physical_text_fragment.h"
#include "core/layout/ng/ng_physical_box_fragment.h"

namespace blink {

namespace {

struct NGInlineFragmentPosition {
  const NGPhysicalFragment* fragment = nullptr;
  NGPhysicalOffset offset_to_container_box;
  NGPhysicalFragment::CaretPositionType position_type;
  Optional<unsigned> text_offset;
};

NGInlineFragmentPosition ComputeNGInlineFragmentPosition(
    const LayoutBlockFlow* context,
    unsigned offset,
    TextAffinity affinity) {
  const NGPhysicalBoxFragment* box_fragment = context->CurrentFragment();
  // TODO(kojii): ...
  if (!box_fragment)
    return NGInlineFragmentPosition();
  Vector<NGInlineFragmentPosition> candidates;
  for (const auto& child :
       NGInlineFragmentTraversal::DescendantsOf(*box_fragment)) {
    if (child.fragment->IsText()) {
      const NGPhysicalTextFragment* text_fragment =
          ToNGPhysicalTextFragment(child.fragment);
      if (offset < text_fragment->StartOffset() ||
          offset > text_fragment->EndOffset()) {
        // TODO(xiaochengh): This may introduce false negatives.
        continue;
      }
      const unsigned offset_in_box = offset - text_fragment->StartOffset();
      NGInlineFragmentPosition candidate = {
          child.fragment, child.offset_to_container_box,
          NGPhysicalFragment::CaretPositionType::kAtTextOffset, offset_in_box};
      if ((offset && offset < text_fragment->Length()) ||
          (!offset && affinity == TextAffinity::kDownstream) ||
          (offset == text_fragment->Length() &&
           affinity == TextAffinity::kUpstream))
        return candidate;
      candidates.push_back(candidate);
    }

    // TODO(xiaochengh): Does this mean atomic inline?
    if (child.fragment->IsBox() && child.fragment->IsInlineBlock()) {
      // TODO(xiaochengh): Design more straightforward way to get text offset of
      // atomic inline box.
      const Node* node = child.fragment->GetNode();
      if (!node) {
        // TODO(xiaochengh): Can we have anonymous atomic inline boxes?
        continue;
      }
      Optional<unsigned> maybe_offset_before =
          NGOffsetMapping::GetFor(context)->GetTextContentOffset(
              Position::BeforeNode(*node));
      if (!maybe_offset_before.has_value()) {
        // TODO(xiaochengh): This shouldn't be possible. Verify.
        continue;
      }
      const unsigned offset_before = maybe_offset_before.value();
      const unsigned offset_after = offset_before + 1;
      if (offset != offset_before && offset != offset_after)
        continue;
      NGInlineFragmentPosition candidate = {
          child.fragment, child.offset_to_container_box,
          offset == offset_before
              ? NGPhysicalFragment::CaretPositionType::kBeforeFragment
              : NGPhysicalFragment::CaretPositionType::kAfterFragment,
          WTF::nullopt};
      if (offset == offset_before) {
        if (affinity == TextAffinity::kDownstream)
          return candidate;
        candidates.push_back(candidate);
        continue;
      }
      if (affinity == TextAffinity::kUpstream)
        return candidate;
      candidates.push_back(candidate);
    }

    if (child.fragment->IsLineBox()) {
      // TODO(xiaochengh): Check if |offset| is in an empty line box.
      // TODO(xiaochengh): Add StartOffset and EndOffset to NGPhysicalLineBox.
    }
  }

  if (candidates.IsEmpty())
    return NGInlineFragmentPosition();

  // TODO(xiaochengh): Choose the correct candidate.
  return candidates.front();
}

}  // namespace

LocalCaretRect ComputeNGLocalCaretRect(const PositionWithAffinity& position) {
  const NGOffsetMapping* mapping =
      NGOffsetMapping::GetFor(position.GetPosition());
  if (!mapping)
    return LocalCaretRect();

  Optional<unsigned> maybe_offset =
      mapping->GetTextContentOffset(position.GetPosition());
  if (!maybe_offset.has_value()) {
    // TODO(xiaochengh): Is this really an expected case?
    return LocalCaretRect();
  }

  const LayoutBlockFlow* context =
      NGInlineFormattingContextOf(position.GetPosition());
  // Non-null |mapping| implies non-null inline formatting context.
  DCHECK(context) << position;

  const unsigned offset = maybe_offset.value();
  const TextAffinity affinity = position.Affinity();

  NGInlineFragmentPosition fragment_position =
      ComputeNGInlineFragmentPosition(context, offset, affinity);
  if (!fragment_position.fragment)
    return LocalCaretRect();

  NGPhysicalOffsetRect fragment_local_rect =
      fragment_position.fragment->LocalCaretRect(
          fragment_position.position_type,
          fragment_position.text_offset.has_value()
              ? &fragment_position.text_offset.value()
              : nullptr);
  NGPhysicalOffsetRect context_local_rect =
      fragment_local_rect + fragment_position.offset_to_container_box;
  return {fragment_position.fragment->GetLayoutObject(),
          context_local_rect.ToLayoutRect()};
}

LocalCaretRect ComputeNGLocalCaretRect(
    const PositionInFlatTreeWithAffinity& position) {
  const PositionWithAffinity dom_position(
      ToPositionInDOMTree(position.GetPosition()), position.Affinity());
  return ComputeNGLocalCaretRect(dom_position);
}

}  // namespace blink
