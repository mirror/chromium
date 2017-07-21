// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/layout/ng/inline/ng_offset_mapping_api.h"

#include "core/dom/Node.h"
#include "core/editing/VisibleUnits.h"
#include "core/layout/ng/inline/ng_offset_mapping_result.h"

namespace blink {

// static
Optional<NGInlineNode> NGOffsetMappingAPI::GetNGInlineNodeFor(
    const Node& node) {
  LayoutObject* layout_object = node.GetLayoutObject();
  if (!layout_object)
    return WTF::nullopt;
  LayoutBox* box = layout_object->EnclosingBox();
  if (!box || !box->ChildrenInline() || !box->IsLayoutNGBlockFlow())
    return WTF::nullopt;
  return NGInlineNode(ToLayoutNGBlockFlow(box));
}

// static
unsigned NGOffsetMappingAPI::ConvertDOMOffsetToTextContent(
    const NGOffsetMappingUnit& unit,
    unsigned offset) {
  DCHECK_GE(offset, unit.dom_start);
  DCHECK_LE(offset, unit.dom_end);
  // DOM start is always mapped to text content start.
  if (offset == unit.dom_start)
    return unit.text_content_start;
  // DOM end is always mapped to text content end.
  if (offset == unit.dom_end)
    return unit.text_content_end;
  // |unit| has collapsed mapping.
  if (unit.text_content_start == unit.text_content_end)
    return unit.text_content_start;
  // |unit| has identity mapping.
  return offset - unit.dom_start + unit.text_content_start;
}

// static
const NGOffsetMappingUnit* NGOffsetMappingAPI::GetMappingUnitForDOMOffset(
    NGInlineNode inline_node,
    const Node& node,
    unsigned offset) {
  LayoutObject* layout_object = AssociatedLayoutObjectOf(node, offset);
  if (!layout_object || !layout_object->IsText())
    return nullptr;

  DCHECK_EQ(layout_object->EnclosingBox(), inline_node.GetLayoutBlockFlow());
  const auto& result = inline_node.ComputeOffsetMappingIfNeeded();

  unsigned range_start;
  unsigned range_end;
  std::tie(range_start, range_end) =
      result.ranges.at(ToLayoutText(layout_object));
  if (range_start == range_end || result.units[range_start].dom_start > offset)
    return nullptr;
  // Find the last unit where unit.dom_start <= offset
  const NGOffsetMappingUnit* unit = std::prev(std::upper_bound(
      result.units.begin() + range_start, result.units.begin() + range_end,
      offset, [](unsigned offset, const NGOffsetMappingUnit& unit) {
        return offset < unit.dom_start;
      }));
  if (unit->dom_end < offset)
    return nullptr;
  return unit;
}

// static
const NGOffsetMappingUnit* NGOffsetMappingAPI::GetMappingUnitForDOMOffset(
    const Node& node,
    unsigned offset) {
  Optional<NGInlineNode> inline_node = GetNGInlineNodeFor(node);
  if (!inline_node)
    return nullptr;
  return GetMappingUnitForDOMOffset(*inline_node, node, offset);
}

// static
const size_t NGOffsetMappingAPI::GetTextContentOffset(const Node& node,
                                                      unsigned offset) {
  const NGOffsetMappingUnit* unit = GetMappingUnitForDOMOffset(node, offset);
  if (!unit)
    return kNotFound;
  return ConvertDOMOffsetToTextContent(*unit, offset);
}

}  // namespace blink
