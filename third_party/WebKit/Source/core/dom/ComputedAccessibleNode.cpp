// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/dom/ComputedAccessibleNode.h"

namespace blink {

ComputedAccessibleNode::ComputedAccessibleNode(Element* elt) {
  DCHECK(RuntimeEnabledFeatures::AccessibilityObjectModelEnabled());
}

std::string ComputedAccessibleNode::name() const {
  return "CAxN name";
}

void ComputedAccessibleNode::setName(const std::string& name) {
  name_ = name;
}

std::string ComputedAccessibleNode::role() const {
  return "CAxN role";
}

void ComputedAccessibleNode::setRole(const std::string& role) {
  role_ = role;
}

}  // namespace blink
