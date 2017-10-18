// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/paint/ng/ng_paint_fragment.h"

#include "core/layout/ng/ng_physical_container_fragment.h"
#include "core/layout/ng/ng_physical_fragment.h"
#include "platform/wtf/PtrUtil.h"

namespace blink {

NGPaintFragment::NGPaintFragment(RefPtr<const NGPhysicalFragment> fragment)
    : physical_fragment_(std::move(fragment)) {
  DCHECK(physical_fragment_);
}

// Populate descendant NGPaintFragment from NGPhysicalFragment tree.
void NGPaintFragment::PopulateDescendants(bool stop_at_block_layout_root) {
  const NGPhysicalFragment& fragment = PhysicalFragment();
  if (fragment.IsContainer() &&
      !(stop_at_block_layout_root && fragment.IsBlockLayoutRoot())) {
    const NGPhysicalContainerFragment& container =
        ToNGPhysicalContainerFragment(fragment);
    children_.ReserveCapacity(container.Children().size());
    for (const auto& child_fragment : container.Children()) {
      auto child = WTF::MakeUnique<NGPaintFragment>(child_fragment);
      child->PopulateDescendants(true);
      children_.push_back(std::move(child));
    }
  }
  // TODO(kojii): Do some stuff to accumulate visual rects and convert to paint
  // coordinates.
  // TODO(kojii): This is still quite worng, revisit in following patch.
  SetVisualRect({{}, fragment.Size().ToLayoutSize()});
}

}  // namespace blink
