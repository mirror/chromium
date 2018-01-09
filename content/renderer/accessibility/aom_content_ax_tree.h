// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_ACCESSIBILITY_AOM_CONTENT_AX_TREE_H_
#define CONTENT_RENDERER_ACCESSIBILITY_AOM_CONTENT_AX_TREE_H_

#include "third_party/WebKit/public/platform/PlatformAXTree.h"

#include <stdint.h>

#include "base/containers/hash_tables.h"
#include "base/macros.h"
#include "content/common/ax_content_node_data.h"
#include "content/renderer/accessibility/render_accessibility_impl.h"
#include "third_party/WebKit/public/platform/PlatformAXNode.h"
#include "ui/accessibility/ax_tree.h"

namespace content {

class AomContentAxTree : public blink::PlatformAXTree {
 public:
  static AomContentAxTree* Create();

  ~AomContentAxTree();

  // blink::PlatformAXTree implementation.
  bool RequestTreeSnapshot() override;
  blink::PlatformAXNode* GetAXNode(int32_t) override;

 private:
  AomContentAxTree();

  ui::AXTree tree_;
  base::hash_map<int32_t, blink::PlatformAXNode*>
      content_platform_node_mapping_;
  DISALLOW_COPY_AND_ASSIGN(AomContentAxTree);
};

#endif  // CONTENT_RENDERER_ACCESSIBILITY_AOM_CONTENT_AX_TREE_H_

}  // namespace content
