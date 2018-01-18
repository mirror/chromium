// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_ACCESSIBILITY_AOM_CONTENT_AX_TREE_H_
#define CONTENT_RENDERER_ACCESSIBILITY_AOM_CONTENT_AX_TREE_H_

#include "third_party/WebKit/public/platform/ComputedAXTree.h"

#include <stdint.h>
#include <string>

#include "base/macros.h"
#include "ui/accessibility/ax_tree.h"

namespace blink {
class WebFrame;
}

namespace content {

class AomContentAxTree : public blink::ComputedAXTree {
 public:
  AomContentAxTree();
  ~AomContentAxTree() override;

  // blink::ComputedAXTree implementation.
  bool ComputeAccessibilityTree(blink::WebFrame*) override;

  // These will return empty strings if the id does not correspond to a node in
  // the current AXTree snapshot.
  const std::string GetNameFromId(int32_t) override;
  const std::string GetRoleFromId(int32_t) override;

  // Get the specified attribute for a given AXID, returning true if the
  // attribute is present on that corresponding AXNode.
  bool GetIntAttributeForId(int32_t, blink::AOMIntAttribute, int32_t*) override;

 private:
  ui::AXTree tree_;

  DISALLOW_COPY_AND_ASSIGN(AomContentAxTree);
};

#endif  // CONTENT_RENDERER_ACCESSIBILITY_AOM_CONTENT_AX_TREE_H_

}  // namespace content
