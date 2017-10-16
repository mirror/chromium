// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NGInlineFragmentIterator_h
#define NGInlineFragmentIterator_h

#include "core/CoreExport.h"
#include "core/layout/ng/geometry/ng_physical_offset.h"
#include "platform/wtf/Allocator.h"
#include "platform/wtf/Vector.h"

namespace blink {

class LayoutObject;
class NGPhysicalContainerFragment;
class NGPhysicalFragment;

class CORE_EXPORT NGInlineFragmentIterator {
  STACK_ALLOCATED();
 public:
  NGInlineFragmentIterator(const NGPhysicalContainerFragment*, const LayoutObject*);

  bool IsAtEnd() const { return container_stack_.IsEmpty(); }
  void MoveToNext();
  std::pair<const NGPhysicalFragment*, NGPhysicalOffset> operator*() const;

 private:
  void MoveToNextIfCurrentIsNotValid();

  struct ContainerStackEntry {
    const NGPhysicalContainerFragment* fragment;
    unsigned child_index;
    NGPhysicalOffset offset_to_container_box;
  };
  Vector<ContainerStackEntry, 4> container_stack_;
  const LayoutObject* target_;
};

}  // namespace blink

#endif  // NGInlineFragmentIterator_h
