// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NGInlineFragmentTraversal_h
#define NGInlineFragmentTraversal_h

#include "core/CoreExport.h"
#include "core/layout/ng/geometry/ng_physical_offset.h"
#include "core/layout/ng/geometry/ng_physical_offset_rect.h"
#include "platform/wtf/Allocator.h"
#include "platform/wtf/Vector.h"

namespace blink {

class NGPhysicalContainerFragment;
class NGPhysicalFragment;

struct NGPhysicalFragmentWithOffset {
  const NGPhysicalFragment* fragment = nullptr;
  NGPhysicalOffset offset_to_container_box;
};

// Iterate through inline descendant fragments.
class CORE_EXPORT NGInlineFragmentTraversal {
  STATIC_ONLY(NGInlineFragmentTraversal);

 public:
  static Vector<const NGPhysicalFragment*> AncestorsOf(
      const NGPhysicalContainerFragment&,
      const NGPhysicalFragment&);
  static Vector<const NGPhysicalFragment*> InclusiveAncestorsOf(
      const NGPhysicalContainerFragment&,
      const NGPhysicalFragment&);
  static Vector<NGPhysicalFragmentWithOffset> DescendantsOf(
      const NGPhysicalContainerFragment&);
  static Vector<NGPhysicalFragmentWithOffset> InclusiveDescendantsOf(
      const NGPhysicalFragment&);
};

}  // namespace blink

#endif  // NGInlineFragmentTraversal_h
