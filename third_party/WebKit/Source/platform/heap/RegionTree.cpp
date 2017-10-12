// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/heap/RegionTree.h"

#include "platform/heap/PageMemory.h"
#include "platform/heap/VisitorImpl.h"
#include "platform/wtf/Assertions.h"

namespace blink {

MemoryRegion* RegionTree::Lookup(Address address) {
  RegionTreeNode* current = root_;
  while (current) {
    Address base = current->region_->Base();
    if (address < base) {
      current = current->left_;
      continue;
    }
    if (address >= base + current->region_->size()) {
      current = current->right_;
      continue;
    }
    DCHECK(current->region_->Contains(address));
    return current->region_;
  }
  return nullptr;
}

RegionTree::~RegionTree() {
  delete root_;
}

void RegionTree::Add(MemoryRegion* region) {
  DCHECK(region);
  RegionTreeNode* new_tree = new RegionTreeNode(region);
  new_tree->AddTo(&root_);
}

void RegionTreeNode::AddTo(RegionTreeNode** context) {
  Address base = region_->Base();
  for (RegionTreeNode* current = *context; current; current = *context) {
    DCHECK(!current->region_->Contains(base));
    context =
        (base < current->region_->Base()) ? &current->left_ : &current->right_;
  }
  *context = this;
}

void RegionTree::Remove(MemoryRegion* region) {
  DCHECK(region);
  DCHECK(root_);
  Address base = region->Base();
  RegionTreeNode** context = &root_;
  RegionTreeNode* current = root_;
  for (; current; current = *context) {
    if (region == current->region_)
      break;
    context =
        (base < current->region_->Base()) ? &current->left_ : &current->right_;
  }

  // Shutdown via detachMainThread might not have populated the region tree.
  if (!current)
    return;

  *context = nullptr;
  if (current->left_) {
    current->left_->AddTo(context);
    current->left_ = nullptr;
  }
  if (current->right_) {
    current->right_->AddTo(context);
    current->right_ = nullptr;
  }
  delete current;
}

}  // namespace blink
