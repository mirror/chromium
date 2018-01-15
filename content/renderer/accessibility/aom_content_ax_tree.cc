// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/accessibility/aom_content_ax_tree.h"

#include <utility>
#include <vector>

#include "base/logging.h"
#include "content/renderer/render_frame_impl.h"
#include "third_party/WebKit/public/web/WebFrame.h"
#include "ui/accessibility/ax_enums.h"

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

ui::AXNode* AomContentAxTree::GetAXNodeFromId(int32_t id) {
  LOG(ERROR) << tree_.ToString();
  auto iter = nodes_.find(id);
  LOG(ERROR) << "Expected id: " << id;
  if (iter == nodes_.end()) {
    LOG(ERROR) << "node is not in local cache";
    ui::AXNode* ax_node = tree_.GetFromId(id);
    if (!ax_node)
      LOG(ERROR) << "ax node not in tree";
    nodes_.insert(std::pair<int32_t, ui::AXNode*>(id, ax_node));
  } else {
    LOG(ERROR) << "node in local cache";
  }

  return nodes_.at(id);
}

const std::string AomContentAxTree::GetNameFromId(int32_t id) {
  ui::AXNode* node = GetAXNodeFromId(id);
  return (node) ? node->data().GetStringAttribute(ui::AX_ATTR_NAME) : "";
}

const std::string AomContentAxTree::GetRoleFromId(int32_t id) {
  ui::AXNode* node = GetAXNodeFromId(id);
  // TODO(meredithl): Change to blink_ax_conversion.cc method once available.
  return (node) ? ui::ToString(node->data().role) : "";
}

}  // namespace content
