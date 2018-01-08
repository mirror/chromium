// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/accessibility/aom_content_ax_tree.h"

#include <vector>

#include "base/logging.h"
#include "content/renderer/render_frame_impl.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"

// static
AomContentAxTree* AomContentAxTree::Create() {
  return new AomContentAxTree();
}

AomContentAxTree::AomContentAxTree() {}

AomContentAxTree::~AomContentAxTree() {}

bool AomContentAxTree::RequestTreeSnapshot() {
  content::AXContentTreeUpdate content_tree_update;
  auto* web_frame = blink::WebLocalFrame::FrameForCurrentContext();
  content::RenderFrameImpl* render_frame =
      content::RenderFrameImpl::FromWebFrame(web_frame);
  content::RenderAccessibilityImpl::SnapshotAccessibilityTree(
      render_frame, &content_tree_update);
  ui::AXTreeData* tree_data = &(content_tree_update.tree_data);

  std::vector<ui::AXNodeData> nodes;
  nodes.assign(content_tree_update.nodes.begin(),
               content_tree_update.nodes.end());
  ui::AXTreeUpdate tree_update;
  tree_update.nodes = nodes;
  tree_update.tree_data = *tree_data;

  return tree_.Unserialize(tree_update);
}

blink::PlatformAXNode* AomContentAxTree::GetAXNode(int32_t id) {
  auto iter = content_platform_node_mapping_.find(id);

  if (iter == content_platform_node_mapping_.end()) {
    AXNode* ax_node = tree_->GetFromId(id);
    AomContentAxNode* aom_node = new AomContentAxNode(ax_node);
    content_platform_node_mapping_.insert(aom_node);
  }
  return content_platform_node_mapping_.at(id);
}
