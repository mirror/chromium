// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/dom/ComputedAccessibleNode.h"

#include <stdint.h>

#include "base/logging.h"
#include "core/dom/DOMException.h"
#include "core/dom/Element.h"
#include "core/frame/Settings.h"
#include "platform/bindings/ScriptState.h"
#include "public/platform/Platform.h"
#include "third_party/WebKit/Source/bindings/core/v8/ScriptPromiseResolver.h"
#include "third_party/WebKit/Source/core/page/Page.h"
#include "third_party/WebKit/Source/modules/accessibility/AXObjectCacheImpl.h"
#include "third_party/WebKit/public/platform/ComputedAXTree.h"
#include "third_party/WebKit/public/web/WebFrame.h"

namespace blink {

ComputedAccessibleNode* ComputedAccessibleNode::Create(AXID id) {
  return new ComputedAccessibleNode(id);
}

ComputedAccessibleNode::ComputedAccessibleNode(AXID id)
    : id_(id), tree_(Platform::Current()->GetOrCreateComputedAXTree()) {
  LOG(ERROR) << "constructing computed ax node with id = " << id;
  DCHECK(RuntimeEnabledFeatures::AccessibilityObjectModelEnabled());
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
    resolver->Reject(DOMException::Create(kUnknownError));
  } else {
    OnSnapshotResponse(resolver);
  }

  return promise;
}

const String ComputedAccessibleNode::name() const {
  return String(tree_->GetNameFromId(id_).c_str());
}

const AtomicString ComputedAccessibleNode::role() const {
  return AtomicString(tree_->GetRoleFromId(id_).c_str());
}

void ComputedAccessibleNode::OnSnapshotResponse(
    ScriptPromiseResolver* resolver) {
  resolver->Resolve(this);
}

}  // namespace blink
