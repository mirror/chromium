// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/dom/ComputedAccessibleNode.h"

#include "bindings/core/v8/ScriptPromise.h"
#include "core/dom/Element.h"
#include "platform/bindings/ScriptState.h"
#include "public/platform/Platform.h"
#include "third_party/WebKit/public/platform/PlatformAXTree.h"

namespace blink {

ComputedAccessibleNode* ComputedAccessibleNode::Create(Element* element) {
  return new ComputedAccessibleNode(element);
}

ComputedAccessibleNode::ComputedAccessibleNode(Element* element)
    : element_(element) {
  DCHECK(RuntimeEnabledFeatures::AccessibilityObjectModelEnabled());
}

ComputedAccessibleNode::~ComputedAccessibleNode() {}

ScriptPromise ComputedAccessibleNode::ComputePromiseProperty(
    ScriptState* script_state) {
  if (!computed_property_) {
    PlatformAXTree* tree_provider = Platform::Current()->PlatformAXTree();
    tree_provider->RequestTreeSnapshot();
    PopulateComputedAccessibleProperties(tree_provider);
    computed_property_ =
        new ComputedPromiseProperty(ExecutionContext::From(script_state), this,
                                    ComputedPromiseProperty::kReady);
    computed_property_->Resolve(this);
  }

  return computed_property_->Promise(script_state->World());
}

void ComputedAccessibleNode::PopulateComputedAccessibleProperties(
    PlatformAXTree* provider) {}

const AtomicString& ComputedAccessibleNode::role() const {
  return element_->computedRole();
}

const String ComputedAccessibleNode::name() const {
  return element_->computedName();
}

void ComputedAccessibleNode::Trace(blink::Visitor* visitor) {
  ScriptWrappable::Trace(visitor);
  visitor->Trace(computed_property_);
  visitor->Trace(element_);
}

}  // namespace blink
