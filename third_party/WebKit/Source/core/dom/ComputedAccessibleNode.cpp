// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/dom/ComputedAccessibleNode.h"

#include <stdint.h>

#include "bindings/core/v8/ScriptPromise.h"
#include "core/dom/DOMException.h"
#include "core/dom/Element.h"
#include "core/frame/Settings.h"
#include "platform/bindings/ScriptState.h"
#include "public/platform/Platform.h"
#include "third_party/WebKit/Source/core/page/Page.h"
#include "third_party/WebKit/Source/modules/accessibility/AXObjectCacheImpl.h"
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
    computed_property_ =
        new ComputedPromiseProperty(ExecutionContext::From(script_state), this,
                                    ComputedPromiseProperty::kReady);

    PlatformAXTree* tree_provider = Platform::Current()->PlatformAXTree();
    if (!tree_provider->RequestTreeSnapshot()) {
      computed_property_->Reject(DOMException::Create(kUnknownError));
    } else {
      PopulateComputedAccessibleProperties(tree_provider);
    }
  }

  return computed_property_->Promise(script_state->World());
}

const String ComputedAccessibleNode::name() const {
  DCHECK(node_);
  return String(node_->GetName().c_str());
}

const AtomicString ComputedAccessibleNode::role() const {
  DCHECK(node_);
  return AtomicString(node_->GetRole().c_str());
}

void ComputedAccessibleNode::Trace(blink::Visitor* visitor) {
  ScriptWrappable::Trace(visitor);
  visitor->Trace(computed_property_);
  visitor->Trace(element_);
}

void ComputedAccessibleNode::PopulateComputedAccessibleProperties(
    PlatformAXTree* provider) {
  element_->GetDocument().GetPage()->GetSettings().SetAccessibilityEnabled(
      true);

  AXObjectCacheImpl* cache =
      ToAXObjectCacheImpl(element_->GetDocument().GetOrCreateAXObjectCache());
  DCHECK(cache);

  AXID element_ax_id = cache->Get(element_)->AXObjectID();
  node_ = provider->GetAXNode(element_ax_id);
  computed_property_->Resolve(this);
}

}  // namespace blink
