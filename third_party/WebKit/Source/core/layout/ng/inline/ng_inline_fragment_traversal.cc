// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/layout/ng/inline/ng_inline_fragment_traversal.h"

#include "core/layout/ng/ng_physical_box_fragment.h"

namespace blink {

namespace {

// Returns true if |container| contains |target|, otherwise false.
bool CollectAncestors(const NGPhysicalContainerFragment& container,
                      const NGPhysicalFragment& target,
                      Vector<const NGPhysicalFragment*>& ancestors) {
  ancestors.push_back(&container);
  for (const auto& child : container.Children()) {
    if (child == &target)
      return true;
    if (!child->IsContainer() || child->IsBlockLayoutRoot())
      continue;
    if (CollectAncestors(ToNGPhysicalContainerFragment(*child), target,
                         ancestors)) {
      return true;
    }
  }
  ancestors.pop_back();
  return false;
}

void CollectDescendants(const NGPhysicalContainerFragment& container,
                        const NGPhysicalOffset parent_offset,
                        Vector<NGPhysicalFragmentWithOffset>& results) {
  for (const auto& child : container.Children()) {
    const NGPhysicalOffset child_offset = parent_offset + child->Offset();
    results.push_back(NGPhysicalFragmentWithOffset{child.get(), child_offset});
    if (!child->IsContainer() || child->IsBlockLayoutRoot())
      continue;
    CollectDescendants(ToNGPhysicalContainerFragment(*child), child_offset,
                       results);
  }
}

}  // anonymous namespace

Vector<const NGPhysicalFragment*> NGInlineFragmentTraversal::AncestorsOf(
    const NGPhysicalContainerFragment& container,
    const NGPhysicalFragment& fragment) {
  Vector<const NGPhysicalFragment*> ancestors;
  CollectAncestors(container, fragment, ancestors);
  ancestors.Reverse();
  return ancestors;
}

Vector<const NGPhysicalFragment*>
NGInlineFragmentTraversal::InclusiveAncestorsOf(
    const NGPhysicalContainerFragment& container,
    const NGPhysicalFragment& fragment) {
  Vector<const NGPhysicalFragment*> results;
  CollectAncestors(container, fragment, results);
  results.push_back(&fragment);
  results.Reverse();
  return results;
}

Vector<NGPhysicalFragmentWithOffset> NGInlineFragmentTraversal::DescendantsOf(
    const NGPhysicalContainerFragment& container) {
  Vector<NGPhysicalFragmentWithOffset> results;
  CollectDescendants(container, {}, results);
  return results;
}

Vector<NGPhysicalFragmentWithOffset>
NGInlineFragmentTraversal::InclusiveDescendantsOf(
    const NGPhysicalFragment& fragment) {
  Vector<NGPhysicalFragmentWithOffset> results;
  results.push_back(NGPhysicalFragmentWithOffset{&fragment, {}});
  if (!fragment.IsContainer() || fragment.IsBlockLayoutRoot())
    return results;
  CollectDescendants(ToNGPhysicalContainerFragment(fragment), {}, results);
  return results;
}

}  // namespace blink
