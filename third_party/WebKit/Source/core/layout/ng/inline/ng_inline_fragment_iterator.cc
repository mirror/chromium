// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/layout/ng/inline/ng_inline_fragment_iterator.h"

#include "core/layout/LayoutObject.h"
#include "core/layout/ng/ng_physical_container_fragment.h"

namespace blink {

NGInlineFragmentIterator::NGInlineFragmentIterator(
    const NGPhysicalContainerFragment* box_fragment,
    const LayoutObject* target)
    : target_(target) {
  DCHECK(box_fragment);
  DCHECK(target);
  container_stack_.push_back(ContainerStackEntry{box_fragment, 0, {}});
  MoveToNextIfCurrentIsNotValid();
}

void NGInlineFragmentIterator::MoveToNext() {
  if (!container_stack_.IsEmpty()) {
    container_stack_.back().child_index++;
    MoveToNextIfCurrentIsNotValid();
  }
}

std::pair<const NGPhysicalFragment*, NGPhysicalOffset> NGInlineFragmentIterator::operator*() const {
  if (container_stack_.IsEmpty())
    return {};
  const ContainerStackEntry& container = container_stack_.back();
  const NGPhysicalFragment* fragment = container.fragment->Children()[container.child_index].get();
  return {fragment, fragment->Offset() + container.offset_to_container_box};
}

void NGInlineFragmentIterator::MoveToNextIfCurrentIsNotValid() {
  for (; !container_stack_.IsEmpty(); ) {
    ContainerStackEntry& container = container_stack_.back();
    for (unsigned child_index = container.child_index; ; child_index++) {
      if (child_index >= container.fragment->Children().size()) {
        container_stack_.pop_back();
        break;
      }
      const NGPhysicalFragment& child = *container.fragment->Children()[child_index];
      const LayoutObject* child_layout_object = child.GetLayoutObject();
      if (child_layout_object == target_) {
        container.child_index = child_index;
        return;
      }
      if (child.IsContainer() && (!child_layout_object || !child_layout_object->IsAtomicInlineLevel() && !child_layout_object->IsFloatingOrOutOfFlowPositioned())) {
        container.child_index = child_index + 1;
        container_stack_.push_back(ContainerStackEntry
            {ToNGPhysicalContainerFragment(&child), 0,
             child.Offset() + container.offset_to_container_box});
        break;
      }
    }
  }
}

}  // namespace blink
