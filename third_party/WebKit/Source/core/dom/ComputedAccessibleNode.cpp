// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/dom/ComputedAccessibleNode.h"

#include <stdint.h>

#include "core/dom/DOMException.h"
#include "platform/bindings/ScriptState.h"
#include "third_party/WebKit/Source/bindings/core/v8/ScriptPromiseResolver.h"
#include "third_party/WebKit/Source/core/frame/LocalFrame.h"
#include "third_party/WebKit/Source/core/frame/WebLocalFrameImpl.h"
#include "third_party/WebKit/Source/platform/wtf/text/WTFString.h"
#include "third_party/WebKit/public/web/WebFrameClient.h"

namespace blink {

ComputedAccessibleNode* ComputedAccessibleNode::Create(Element* element) {
  return new ComputedAccessibleNode(element);
}

ComputedAccessibleNode::ComputedAccessibleNode(Element* element)
    : element_(element) {
  DCHECK(RuntimeEnabledFeatures::AccessibilityObjectModelEnabled());
  AXObjectCache* cache = element->GetDocument().GetOrCreateAXObjectCache();
  DCHECK(cache);
  cache_ = cache;

  LocalFrame* local_frame = element->ownerDocument()->GetFrame();
  WebFrameClient* client = WebLocalFrameImpl::FromFrame(local_frame)->Client();
  tree_ = client->GetOrCreateWebComputedAXTree();
}

ComputedAccessibleNode::~ComputedAccessibleNode() {}

ScriptPromise ComputedAccessibleNode::ComputeAccessibleProperties(
    ScriptState* script_state) {
  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  ScriptPromise promise = resolver->Promise();
  // TODO(meredithl): Post this task asynchronously, with a callback into
  // this->OnSnapshotResponse.
  if (!tree_->ComputeAccessibilityTree()) {
    // TODO(meredithl): Change this exception to something relevant to AOM.
    resolver->Reject(DOMException::Create(kUnknownError));
  } else {
    OnSnapshotResponse(resolver);
  }

  return promise;
}

void ComputedAccessibleNode::OnSnapshotResponse(
    ScriptPromiseResolver* resolver) {
  resolver->Resolve(this);
}

const String ComputedAccessibleNode::keyShortcuts() const {
  return tree_->GetStringAttributeForAXNode(
      cache_->GetAXID(element_), AOMStringAttribute::AOM_ATTR_KEY_SHORTCUTS);
}
const String ComputedAccessibleNode::name() const {
  return tree_->GetStringAttributeForAXNode(cache_->GetAXID(element_),
                                            AOMStringAttribute::AOM_ATTR_NAME);
}
const String ComputedAccessibleNode::placeholder() const {
  return tree_->GetStringAttributeForAXNode(
      cache_->GetAXID(element_), AOMStringAttribute::AOM_ATTR_PLACEHOLDER);
}
const String ComputedAccessibleNode::role() const {
  return tree_->GetStringAttributeForAXNode(cache_->GetAXID(element_),
                                            AOMStringAttribute::AOM_ATTR_ROLE);
}
const String ComputedAccessibleNode::roleDescription() const {
  return tree_->GetStringAttributeForAXNode(
      cache_->GetAXID(element_), AOMStringAttribute::AOM_ATTR_ROLE_DESCRIPTION);
}
const String ComputedAccessibleNode::valueText() const {
  return tree_->GetStringAttributeForAXNode(
      cache_->GetAXID(element_), AOMStringAttribute::AOM_ATTR_VALUE_TEXT);
}

void ComputedAccessibleNode::Trace(Visitor* visitor) {
  ScriptWrappable::Trace(visitor);
  visitor->Trace(element_);
  visitor->Trace(cache_);
}

}  // namespace blink
