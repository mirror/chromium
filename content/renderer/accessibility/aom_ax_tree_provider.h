// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_ACCESSIBILITY_AOM_AX_TREE_PROVIDER_H_
#define CONTENT_RENDERER_ACCESSIBILITY_AOM_AX_TREE_PROVIDER_H_

#include "third_party/WebKit/public/platform/PlatformAXTree.h"

#include "base/macros.h"
#include "content/common/ax_content_node_data.h"
#include "content/renderer/accessibility/render_accessibility_impl.h"
#include "ui/accessibility/ax_tree.h"

class AomAxTreeProvider : public blink::PlatformAXTree {
 public:
  static AomAxTreeProvider* Create();

  ~AomAxTreeProvider();

  // blink::PlatformAXTree implementation.
  bool RequestTreeSnapshot() override;

  // TODO(meredithl): Figure out what the mapping is from the blink::Element and
  // the AXNodeData.
  void GetAXNode(/* Element -> AXNodeData identifier */) override;

 private:
  AomAxTreeProvider();

  ui::AXTree tree_;
  DISALLOW_COPY_AND_ASSIGN(AomAxTreeProvider);
};

#endif  // CONTENT_RENDERER_ACCESSIBILITY_AOM_AX_TREE_PROVIDER_H_
