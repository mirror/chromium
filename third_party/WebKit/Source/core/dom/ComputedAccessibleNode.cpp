// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/dom/ComputedAccessibleNode.h"

#include <stdint.h>

#include "core/dom/AXObjectCache.h"
#include "core/dom/DOMException.h"
#include "platform/bindings/ScriptState.h"
#include "public/platform/Platform.h"
#include "third_party/WebKit/Source/bindings/core/v8/ScriptPromiseResolver.h"
#include "third_party/WebKit/public/web/WebFrame.h"

namespace blink {

ComputedAccessibleNode* ComputedAccessibleNode::Create(Element* element) {
  return new ComputedAccessibleNode(element);
}

ComputedAccessibleNode::ComputedAccessibleNode(Element* element)
    : element_(element),
      tree_(Platform::Current()->GetOrCreateComputedAXTree()) {
  DCHECK(RuntimeEnabledFeatures::AccessibilityObjectModelEnabled());
  AXObjectCache* cache = element->GetDocument().GetOrCreateAXObjectCache();
  DCHECK(cache);
  cache_ = cache;
}

ComputedAccessibleNode::~ComputedAccessibleNode() {}

ScriptPromise ComputedAccessibleNode::ComputeAccessibleProperties(
    ScriptState* script_state,
    WebFrame* frame) {
  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  ScriptPromise promise = resolver->Promise();
  // TODO(meredithl): Post this task asynchronously, with a callback into
  // this->OnSnapshotResponse.
  if (!tree_->ComputeAccessibilityTree(frame)) {
    // TODO(meredithl): Change this exception to something relevant to AOM.
    resolver->Reject(DOMException::Create(kUnknownError));
  } else {
    OnSnapshotResponse(resolver);
  }

  return promise;
}

const String ComputedAccessibleNode::name() const {
  return String(tree_->GetNameForAXNode(cache_->GetAXID(element_)).c_str());
}

const AtomicString ComputedAccessibleNode::role() const {
  return AtomicString(
      tree_->GetRoleForAXNode(cache_->GetAXID(element_)).c_str());
}

void ComputedAccessibleNode::OnSnapshotResponse(
    ScriptPromiseResolver* resolver) {
  resolver->Resolve(this);
}

void ComputedAccessibleNode::Trace(Visitor* visitor) {
  ScriptWrappable::Trace(visitor);
  visitor->Trace(element_);
  visitor->Trace(cache_);
}

}  // namespace blink
