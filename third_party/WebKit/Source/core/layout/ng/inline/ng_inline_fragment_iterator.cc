// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/layout/ng/inline/ng_inline_fragment_iterator.h"

#include "core/layout/LayoutObject.h"
#include "core/layout/ng/ng_physical_container_fragment.h"

namespace blink {

namespace {

bool IsLayoutNGBlockLayoutBoundary(const LayoutObject* layout_object) {
  return layout_object && (layout_object->IsAtomicInlineLevel() ||
                           layout_object->IsFloatingOrOutOfFlowPositioned());
}

}  // namespace

NGInlineFragmentIterator::NGInlineFragmentIterator(
    const NGPhysicalContainerFragment* container,
    const LayoutObject* target)
    : target_(target) {
  DCHECK(container);
  DCHECK(target);
  container_stack_.push_back(ContainerStackEntry{container, 0, {}});
  MoveToNextIfCurrentIsNotValid();
}

void NGInlineFragmentIterator::operator++() {
  if (!container_stack_.IsEmpty()) {
    container_stack_.back().child_index++;
    MoveToNextIfCurrentIsNotValid();
  }
}

NGInlineFragmentIterator::Result NGInlineFragmentIterator::operator*() const {
  DCHECK(!container_stack_.IsEmpty());
  const ContainerStackEntry& container = container_stack_.back();
  const NGPhysicalFragment& fragment =
      *container.fragment->Children()[container.child_index];
  return {fragment, fragment.Offset() + container.offset_to_container_box};
}

void NGInlineFragmentIterator::MoveToNextIfCurrentIsNotValid() {
  for (; !container_stack_.IsEmpty();) {
    ContainerStackEntry& container = container_stack_.back();
    const auto& children = container.fragment->Children();
    for (unsigned child_index = container.child_index;; child_index++) {
      if (child_index >= children.size()) {
        container_stack_.pop_back();
        break;
      }

      const NGPhysicalFragment& child = *children[child_index];
      const LayoutObject* child_layout_object = child.GetLayoutObject();
      if (child_layout_object == target_) {
        container.child_index = child_index;
        return;
      }

      if (child.IsContainer() &&
          !IsLayoutNGBlockLayoutBoundary(child_layout_object)) {
        container.child_index = child_index + 1;
        container_stack_.push_back(ContainerStackEntry{
            ToNGPhysicalContainerFragment(&child), 0,
            child.Offset() + container.offset_to_container_box});
        break;
      }
    }
  }
}

}  // namespace blink
