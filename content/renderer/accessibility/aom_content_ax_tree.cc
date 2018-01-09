// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/accessibility/aom_content_ax_tree.h"

#include <utility>
#include <vector>

#include "base/logging.h"
#include "content/renderer/accessibility/aom_content_ax_node.h"
#include "content/renderer/render_frame_impl.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"

namespace content {

// static
AomContentAxTree* AomContentAxTree::Create() {
  return new AomContentAxTree();
}

AomContentAxTree::AomContentAxTree() {}

AomContentAxTree::~AomContentAxTree() {}

bool AomContentAxTree::RequestTreeSnapshot() {
  AXContentTreeUpdate content_tree_update;
  auto* web_frame = blink::WebLocalFrame::FrameForCurrentContext();
  RenderFrameImpl* render_frame = RenderFrameImpl::FromWebFrame(web_frame);
  RenderAccessibilityImpl::SnapshotAccessibilityTree(render_frame,
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

  std::vector<ui::AXNodeData> nodes;
  nodes.assign(content_tree_update.nodes.begin(),
               content_tree_update.nodes.end());
  tree_update.nodes = nodes;

  return tree_.Unserialize(tree_update);
}

blink::PlatformAXNode* AomContentAxTree::GetAXNode(int32_t id) {
  auto iter = content_platform_node_mapping_.find(id);

  if (iter == content_platform_node_mapping_.end()) {
    ui::AXNode* ax_node = tree_.GetFromId(id);
    blink::PlatformAXNode* aom_node = new AomContentAxNode(ax_node);
    content_platform_node_mapping_.insert(
        std::pair<int32_t, blink::PlatformAXNode*>(id, aom_node));
  }
  return content_platform_node_mapping_.at(id);
}

}  // namespace content
