// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/accessibility/aom_content_ax_node.h"
#include "ui/accessibility/ax_enums.h"

AomAxContentNode::AomAxContentNode(AXNode* ax_node) : node_(ax_node) {}

std::string& AomAxContentNode::GetStringAttribute(
    blink::PlatformAXStringAttribute attr) {
  return node_->data().GetStringAttribute((AXStringAttribute)attr);
}
