// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/accessibility/aom_content_ax_node.h"
#include "ui/accessibility/ax_enums.h"

namespace content {

AomContentAxNode::AomContentAxNode(ui::AXNode* ax_node) : node_(ax_node) {}

bool AomContentAxNode::GetScrollable() {
  return node_->data().GetBoolAttribute(ui::AX_ATTR_SCROLLABLE);
}

float AomContentAxNode::GetFontSize() {
  return node_->data().GetFloatAttribute(ui::AX_ATTR_FONT_SIZE);
}

int AomContentAxNode::GetColorValue() {
  return node_->data().GetIntAttribute(ui::AX_ATTR_COLOR_VALUE);
}

const std::string& AomContentAxNode::GetName() {
  return node_->data().GetStringAttribute(ui::AX_ATTR_NAME);
}

const std::string& AomContentAxNode::GetRole() {
  return node_->data().GetStringAttribute(ui::AX_ATTR_ROLE);
}

const std::vector<int32_t>& AomContentAxNode::GetLabelledByIds() {
  return node_->data().GetIntListAttribute(ui::AX_ATTR_LABELLEDBY_IDS);
}

}  // namespace content
