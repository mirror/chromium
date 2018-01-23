// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_ACCESSIBILITY_AOM_CONTENT_AX_TREE_H_
#define CONTENT_RENDERER_ACCESSIBILITY_AOM_CONTENT_AX_TREE_H_

#include "third_party/WebKit/public/platform/WebComputedAXTree.h"
#include "third_party/WebKit/public/platform/WebString.h"

#include <stdint.h>
#include <string>

#include "base/macros.h"
#include "ui/accessibility/ax_tree.h"

namespace blink {
class WebFrame;
}

namespace content {

class AomContentAxTree : public blink::WebComputedAXTree {
 public:
  AomContentAxTree();
  ~AomContentAxTree() override;

  // blink::WebComputedAXTree implementation.
  bool ComputeAccessibilityTree(blink::WebFrame*) override;

  // String accessible property getters.
  // TODO(meredithl): Change these to return null rather than empty strings.
  blink::WebString GetNameForAXNode(int32_t) override;
  blink::WebString GetRoleForAXNode(int32_t) override;

 private:
  ui::AXTree tree_;

  DISALLOW_COPY_AND_ASSIGN(AomContentAxTree);
};

#endif  // CONTENT_RENDERER_ACCESSIBILITY_AOM_CONTENT_AX_TREE_H_

}  // namespace content
