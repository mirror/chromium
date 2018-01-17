// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/dom/ComputedAccessibleNode.h"

#include <stdint.h>

#include "core/dom/AXObjectCache.h"
#include "core/dom/DOMException.h"
#include "platform/bindings/ScriptState.h"
#include "public/platform/AOMEnums.h"
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
  return String(tree_->GetNameFromId(cache_->GetAXID(element_)).c_str());
}

const AtomicString ComputedAccessibleNode::role() const {
  return AtomicString(tree_->GetRoleFromId(cache_->GetAXID(element_)).c_str());
}

int32_t ComputedAccessibleNode::colCount() const {
  return tree_->GetIntAttributeForId(cache_->GetAXID(element_),
                                     AOMIntAttribute::AOM_ATTR_COLUMN_COUNT);
}

int32_t ComputedAccessibleNode::colIndex() const {
  return tree_->GetIntAttributeForId(cache_->GetAXID(element_),
                                     AOMIntAttribute::AOM_ATTR_COLUMN_INDEX);
}

int32_t ComputedAccessibleNode::colSpan() const {
  return tree_->GetIntAttributeForId(cache_->GetAXID(element_),
                                     AOMIntAttribute::AOM_ATTR_COLUMN_SPAN);
}

int32_t ComputedAccessibleNode::level() const {
  return tree_->GetIntAttributeForId(
      cache_->GetAXID(element_), AOMIntAttribute::AOM_ATTR_HIERARCHICAL_LEVEL);
}

int32_t ComputedAccessibleNode::posInSet() const {
  return tree_->GetIntAttributeForId(cache_->GetAXID(element_),
                                     AOMIntAttribute::AOM_ATTR_POS_IN_SET);
}

int32_t ComputedAccessibleNode::rowCount() const {
  return tree_->GetIntAttributeForId(cache_->GetAXID(element_),
                                     AOMIntAttribute::AOM_ATTR_ROW_COUNT);
}

int32_t ComputedAccessibleNode::rowIndex() const {
  return tree_->GetIntAttributeForId(cache_->GetAXID(element_),
                                     AOMIntAttribute::AOM_ATTR_ROW_INDEX);
}

int32_t ComputedAccessibleNode::rowSpan() const {
  return tree_->GetIntAttributeForId(cache_->GetAXID(element_),
                                     AOMIntAttribute::AOM_ATTR_ROW_SPAN);
}

int32_t ComputedAccessibleNode::setSize() const {
  return tree_->GetIntAttributeForId(cache_->GetAXID(element_),
                                     AOMIntAttribute::AOM_ATTR_SET_SIZE);
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
