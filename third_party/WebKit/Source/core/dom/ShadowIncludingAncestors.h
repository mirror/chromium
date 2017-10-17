// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ShadowIncludingAncestors_h
#define ShadowIncludingAncestors_h

#include "core/CoreExport.h"
#include "core/dom/Node.h"
#include "platform/wtf/Vector.h"

namespace blink {

class CORE_EXPORT ShadowIncludingAncestors {
  DISALLOW_NEW();

 public:
  void Reduce(Node* new_leaf);
  void Clear();
  Node* LastNode();
  Element* PopElement();
  const HeapVector<Member<Node>>& AncestorVector();

  DECLARE_TRACE();

 private:
  void BuildAncestorChain(Node* leaf);

  Member<Node> leaf_;
  HeapVector<Member<Node>> ancestors_;
};

}  // namespace blink

#endif  // ShadowIncludingAncestors_h
