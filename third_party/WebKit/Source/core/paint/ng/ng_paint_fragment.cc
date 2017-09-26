// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/paint/ng/ng_paint_fragment.h"

#include "core/layout/ng/ng_physical_fragment.h"
#include "core/layout/ng/ng_physical_container_fragment.h"
#include "platform/wtf/PtrUtil.h"

namespace blink {

NGPaintFragment::NGPaintFragment(RefPtr<const NGPhysicalFragment> fragment)
      : physical_fragment_(fragment) {
  DCHECK(fragment);
  PopulateDescendants();
}

void NGPaintFragment::PopulateDescendants() {
  if (PhysicalFragment().IsContainer()) {
    const NGPhysicalContainerFragment& fragment =
        ToNGPhysicalContainerFragment(PhysicalFragment());
    for (const auto& child_fragment : fragment.Children()) {
      auto child = WTF::MakeUnique<NGPaintFragment>(child_fragment);
      //      child->PopulateDescendants();
      children_.push_back(std::move(child));
    }
  }
  // Do some stuff to convert to paint coordinates.
  SetVisualRect(PhysicalFragment().VisualRect());
}

}  // namespace blink
