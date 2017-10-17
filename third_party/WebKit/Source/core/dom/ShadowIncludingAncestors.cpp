// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/dom/ShadowIncludingAncestors.h"

#include "core/dom/ContainerNode.h"
#include "core/dom/Element.h"
#include "platform/heap/TraceTraits.h"

namespace blink {

void ShadowIncludingAncestors::Reduce(Node* new_leaf) {
  DCHECK(new_leaf);
  if (leaf_) {
    DCHECK(ancestors_.IsEmpty());
    BuildAncestorChain(leaf_);
    leaf_ = nullptr;
  } else if (ancestors_.IsEmpty()) {
    leaf_ = new_leaf;
    return;
  }

  size_t index = ancestors_.ReverseFind(new_leaf);
  DCHECK(index != kNotFound);
  if (index != kNotFound)
    ancestors_.Shrink(index + 1);
}

void ShadowIncludingAncestors::BuildAncestorChain(Node* new_leaf) {
  if (new_leaf) {
    BuildAncestorChain(new_leaf->ParentOrShadowHostNode());
    ancestors_.push_back(new_leaf);
  }
}

Node* ShadowIncludingAncestors::LastNode() {
  if (leaf_)
    return leaf_.Get();
  if (ancestors_.IsEmpty())
    return nullptr;
  return ancestors_.back();
}

void ShadowIncludingAncestors::Clear() {
  leaf_ = nullptr;
  ancestors_.clear();
}

const HeapVector<Member<Node>>& ShadowIncludingAncestors::AncestorVector() {
  if (leaf_) {
    DCHECK(ancestors_.IsEmpty());
    BuildAncestorChain(leaf_);
    leaf_ = nullptr;
  }
  return ancestors_;
}

Element* ShadowIncludingAncestors::PopElement() {
  if (leaf_) {
    DCHECK(ancestors_.IsEmpty());
    BuildAncestorChain(leaf_);
    leaf_ = nullptr;
  }
  auto last_element_it = ancestors_.end();
  for (auto it = ancestors_.begin(); it != ancestors_.end(); ++it) {
    Node* node = *it;
    if (node->NeedsAttach())
      break;
    if (node->IsElementNode())
      last_element_it = it;
  }
  if (last_element_it != ancestors_.end()) {
    Element* last_element = ToElement(*last_element_it);
    ancestors_.Shrink(last_element_it - ancestors_.begin());
    return last_element;
  }
  return nullptr;
}

DEFINE_TRACE(ShadowIncludingAncestors) {
  visitor->Trace(leaf_);
  visitor->Trace(ancestors_);
}

}  // namespace blink
