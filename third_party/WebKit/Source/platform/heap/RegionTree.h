// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef RegionTree_h
#define RegionTree_h

#include "platform/PlatformExport.h"
#include "platform/heap/BlinkGC.h"
#include "platform/wtf/Allocator.h"

namespace blink {

class MemoryRegion;
class RegionTreeNode;

// A RegionTree is a simple binary search tree of MemoryRegions sorted
// by base addresses.
class PLATFORM_EXPORT RegionTree {
  USING_FAST_MALLOC(RegionTree);

 public:
  RegionTree() : root_(nullptr) {}
  ~RegionTree();

  void Add(MemoryRegion*);
  void Remove(MemoryRegion*);
  MemoryRegion* Lookup(Address);

 private:
  RegionTreeNode* root_;
};

class RegionTreeNode {
  USING_FAST_MALLOC(RegionTreeNode);

 public:
  explicit RegionTreeNode(MemoryRegion* region)
      : region_(region), left_(nullptr), right_(nullptr) {}

  ~RegionTreeNode() {
    delete left_;
    delete right_;
  }

  void AddTo(RegionTreeNode** context);

 private:
  MemoryRegion* region_;
  RegionTreeNode* left_;
  RegionTreeNode* right_;

  friend RegionTree;
};

}  // namespace blink

#endif  // RegionTree_h
