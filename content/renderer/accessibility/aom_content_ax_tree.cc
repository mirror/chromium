// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/accessibility/aom_content_ax_tree.h"

#include "content/common/ax_content_node_data.h"
#include "content/renderer/accessibility/render_accessibility_impl.h"
#include "content/renderer/render_frame_impl.h"
#include "third_party/WebKit/public/web/WebFrame.h"
#include "ui/accessibility/ax_enums.h"
#include "ui/accessibility/ax_node.h"

namespace {

ui::AXIntAttribute ParseAOMIntAttribute(blink::AOMIntAttribute attr) {
  switch (attr) {
    case blink::AOMIntAttribute::AOM_ATTR_COLUMN_COUNT:
      return ui::AXIntAttribute::AX_ATTR_TABLE_COLUMN_COUNT;
    case blink::AOMIntAttribute::AOM_ATTR_COLUMN_INDEX:
      return ui::AXIntAttribute::AX_ATTR_TABLE_COLUMN_INDEX;
    case blink::AOMIntAttribute::AOM_ATTR_COLUMN_SPAN:
      return ui::AXIntAttribute::AX_ATTR_TABLE_CELL_COLUMN_SPAN;
    case blink::AOMIntAttribute::AOM_ATTR_HIERARCHICAL_LEVEL:
      return ui::AXIntAttribute::AX_ATTR_HIERARCHICAL_LEVEL;
    case blink::AOMIntAttribute::AOM_ATTR_POS_IN_SET:
      return ui::AXIntAttribute::AX_ATTR_POS_IN_SET;
    case blink::AOMIntAttribute::AOM_ATTR_ROW_COUNT:
      return ui::AXIntAttribute::AX_ATTR_TABLE_ROW_COUNT;
    case blink::AOMIntAttribute::AOM_ATTR_ROW_INDEX:
      return ui::AXIntAttribute::AX_ATTR_TABLE_ROW_INDEX;
    case blink::AOMIntAttribute::AOM_ATTR_ROW_SPAN:
      return ui::AXIntAttribute::AX_ATTR_TABLE_CELL_ROW_SPAN;
    case blink::AOMIntAttribute::AOM_ATTR_SET_SIZE:
      return ui::AXIntAttribute::AX_ATTR_SET_SIZE;
    default:
      return ui::AXIntAttribute::AX_INT_ATTRIBUTE_NONE;
  }
}

ui::AXStringAttribute ParseAOMStringAttribute(blink::AOMStringAttribute attr) {
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

AomContentAxTree::AomContentAxTree() {}

AomContentAxTree::~AomContentAxTree() {}

bool AomContentAxTree::ComputeAccessibilityTree(blink::WebFrame* web_frame) {
  AXContentTreeUpdate content_tree_update;
  RenderAccessibilityImpl::SnapshotAccessibilityTree(
      RenderFrameImpl::FromWebFrame(web_frame), &content_tree_update);

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

bool AomContentAxTree::GetIntAttributeForAXNode(int32_t axID,
                                                blink::AOMIntAttribute attr,
                                                int32_t* out) {
  ui::AXNode* node = tree_.GetFromId(axID);
  if (!node)
    return false;
  return node->data().GetIntAttribute(ParseAOMIntAttribute(attr), out);
}

bool AomContentAxTree::GetStringAttributeForAXNode(
    int32_t axID,
    blink::AOMStringAttribute attr,
    std::string* out) {

  ui::AXNode* node = tree_.GetFromId(axID);
  if (!node)
    return false;

  // The role is stored seperately inside AXNodeData, so we have to do a special
  // case for this attribute.
  if (attr == blink::AOMStringAttribute::AOM_ATTR_ROLE) {
    // TODO(meredithl): Change to blink_ax_conversion.cc method once available.
    *out = std::string(ui::ToString(node->data().role));
    return true;
  }

  if (attr == blink::AOMStringAttribute::AOM_ATTR_ROLE_DESCRIPTION) {
    LOG(ERROR) << tree_.GetFromId(axID)->data().ToString();
  }

  return node->data().GetStringAttribute(ParseAOMStringAttribute(attr), out);
}

}  // namespace content
