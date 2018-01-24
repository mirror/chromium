// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/accessibility/aom_content_ax_tree.h"

#include <string>

#include "content/common/ax_content_node_data.h"
#include "content/renderer/accessibility/render_accessibility_impl.h"
#include "ui/accessibility/ax_enums.h"
#include "ui/accessibility/ax_node.h"

namespace {

ui::AXStringAttribute GetCorrespondingAXAttribute(
    blink::AOMStringAttribute attr) {
  switch (attr) {
    case blink::AOMStringAttribute::AOM_ATTR_KEY_SHORTCUTS:
      return ui::AXStringAttribute::AX_ATTR_KEY_SHORTCUTS;
    case blink::AOMStringAttribute::AOM_ATTR_NAME:
      return ui::AXStringAttribute::AX_ATTR_NAME;
    case blink::AOMStringAttribute::AOM_ATTR_PLACEHOLDER:
      return ui::AXStringAttribute::AX_ATTR_PLACEHOLDER;
    case blink::AOMStringAttribute::AOM_ATTR_ROLE_DESCRIPTION:
      return ui::AXStringAttribute::AX_ATTR_ROLE_DESCRIPTION;
    case blink::AOMStringAttribute::AOM_ATTR_VALUE_TEXT:
      return ui::AXStringAttribute::AX_ATTR_VALUE;
    default:
      return ui::AXStringAttribute::AX_STRING_ATTRIBUTE_NONE;
  }
}

}  // namespace

namespace content {

AomContentAxTree::AomContentAxTree(RenderFrameImpl* render_frame)
    : render_frame_(render_frame) {}

bool AomContentAxTree::ComputeAccessibilityTree() {
  AXContentTreeUpdate content_tree_update;
  RenderAccessibilityImpl::SnapshotAccessibilityTree(render_frame_,
                                                     &content_tree_update);

  // Hack to convert between AXContentNodeData and AXContentTreeData to just
  // AXNodeData and AXTreeData to preserve content specific attributes while
  // still being able to use AXTree's Unserialize method.
  ui::AXTreeUpdate tree_update;
  tree_update.has_tree_data = content_tree_update.has_tree_data;
  ui::AXTreeData* tree_data = &(content_tree_update.tree_data);
  tree_update.tree_data = *tree_data;
  tree_update.node_id_to_clear = content_tree_update.node_id_to_clear;
  tree_update.root_id = content_tree_update.root_id;
  tree_update.nodes.assign(content_tree_update.nodes.begin(),
                           content_tree_update.nodes.end());
  return tree_.Unserialize(tree_update);
}

blink::WebString AomContentAxTree::GetStringAttributeForAXNode(
    int32_t axID,
    blink::AOMStringAttribute attr) {
  ui::AXNode* node = tree_.GetFromId(axID);
  if (!node)
    return blink::WebString();

  // The role is stored seperately inside AXNodeData, so we have to do a special
  // case for this attribute.
  if (attr == blink::AOMStringAttribute::AOM_ATTR_ROLE) {
    // TODO(meredithl): Change to blink_ax_conversion.cc method once available.
    return blink::WebString::FromUTF8(ui::ToString(node->data().role));
  }

  std::string out_string;
  if (node->data().GetStringAttribute(GetCorrespondingAXAttribute(attr),
                                      &out_string)) {
    return blink::WebString::FromUTF8(out_string.c_str());
  }

  return blink::WebString();
}

}  // namespace content
