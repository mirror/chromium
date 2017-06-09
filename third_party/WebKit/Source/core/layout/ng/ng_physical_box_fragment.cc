// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/layout/ng/ng_physical_box_fragment.h"

#include "core/layout/ng/ng_unpositioned_float.h"

namespace blink {

NGPhysicalBoxFragment::NGPhysicalBoxFragment(
    LayoutObject* layout_object,
    NGPhysicalSize size,
    NGPhysicalSize overflow,
    Vector<RefPtr<NGPhysicalFragment>>& children,
    const Vector<NGPositionedFloat>& positioned_floats,
    const WTF::Optional<NGLogicalOffset>& bfc_offset,
    const NGMarginStrut& end_margin_strut,
    unsigned border_edges,  // NGBorderEdges::Physical
    RefPtr<NGBreakToken> break_token)
    : NGPhysicalFragment(layout_object,
                         size,
                         kFragmentBox,
                         std::move(break_token)),
      overflow_(overflow),
      positioned_floats_(positioned_floats),
      bfc_offset_(bfc_offset),
      end_margin_strut_(end_margin_strut) {
  children_.swap(children);
  border_edge_ = border_edges;
}

RefPtr<NGPhysicalFragment> NGPhysicalBoxFragment::Clone() const {
  Vector<RefPtr<NGPhysicalFragment>> children_copy(children_);
  RefPtr<NGPhysicalFragment> physical_fragment =
      AdoptRef(new NGPhysicalBoxFragment(
          layout_object_, size_, overflow_, children_copy, positioned_floats_,
          bfc_offset_, end_margin_strut_, border_edge_, break_token_));
  if (is_placed_)
    physical_fragment->SetOffset(offset_);
  return physical_fragment;
}

}  // namespace blink
