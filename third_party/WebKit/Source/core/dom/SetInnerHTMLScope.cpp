// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/dom/SetInnerHTMLScope.h"
#include "core/dom/ChildNodeList.h"
#include "core/dom/ContainerNode.h"
#include "core/dom/NodeListsNodeData.h"

namespace blink {

int SetInnerHTMLScope::count_ = 0;

SetInnerHTMLScope::SetInnerHTMLScope(ContainerNode* node) {
  node->InvalidateNodeListCachesInAncestors();
  count_++;
}

}  // namespace blink
