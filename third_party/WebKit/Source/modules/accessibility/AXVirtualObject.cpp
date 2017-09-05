// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/accessibility/AXVirtualObject.h"

namespace blink {

AXVirtualObject::AXVirtualObject(AXObjectCacheImpl& axObjectCache,
                                 AccessibleNode* accessible_node)
    : AXObject(axObjectCache), accessible_node_(accessible_node) {
  LOG(ERROR) << "AXVirtualObject ctor";
}

AXVirtualObject::~AXVirtualObject() {}

bool AXVirtualObject::ComputeAccessibilityIsIgnored(
    IgnoredReasons* ignoredReasons) const {
  return AccessibilityIsIgnoredByDefault(ignoredReasons);
}

void AXVirtualObject::AddChildren() {
  AddAccessibleNodeChildren();
}

const AtomicString& AXVirtualObject::GetAOMPropertyOrARIAAttribute(
    AOMStringProperty property) const {
  return accessible_node_->GetProperty(property);
}

DEFINE_TRACE(AXVirtualObject) {
  visitor->Trace(accessible_node_);
  AXObject::Trace(visitor);
}

}  // namespace blink
