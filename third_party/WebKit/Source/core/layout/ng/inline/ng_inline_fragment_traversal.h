// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NGInlineFragmentTraversal_h
#define NGInlineFragmentTraversal_h

#include "core/CoreExport.h"
#include "core/layout/ng/ng_physical_fragment.h"
#include "platform/wtf/Allocator.h"
#include "platform/wtf/Vector.h"

namespace blink {

class NGPhysicalContainerFragment;
class NGPhysicalFragment;

// Iterate through inline descendant fragments.
class CORE_EXPORT NGInlineFragmentTraversal {
  STATIC_ONLY(NGInlineFragmentTraversal);

 public:
  // Return list of ancestors from |target| to |container|. Offsets are relative
  // to |container|.
  static Vector<NGPhysicalFragmentWithOffset> AncestorsOf(
      const NGPhysicalContainerFragment& container,
      const NGPhysicalFragment& target);

  // Return list inclusive ancestors from |target| to |container|. Offsets are
  // relative to |container|.
  static Vector<NGPhysicalFragmentWithOffset> InclusiveAncestorsOf(
      const NGPhysicalContainerFragment& container,
      const NGPhysicalFragment& target);

  // Returns list of descendants in DFS order. Offsets are relative to
  // specified fragment.
  static Vector<NGPhysicalFragmentWithOffset> DescendantsOf(
      const NGPhysicalContainerFragment&);

  // Returns list of inclusive descendants in DFS order. Offsets are relative to
  // specified fragment.
  static Vector<NGPhysicalFragmentWithOffset> InclusiveDescendantsOf(
      const NGPhysicalFragment&);
};

}  // namespace blink

#endif  // NGInlineFragmentTraversal_h
