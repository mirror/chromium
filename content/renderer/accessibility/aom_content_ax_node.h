// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_ACCESSIBILITY_AOM_CONTENT_AX_NODE_H_
#define CONTENT_RENDERER_ACCESSIBILITY_AOM_CONTENT_AX_NODE_H_

#include <string>

#include "third_party/WebKit/public/platform/PlatformAXNode.h"
#include "ui/accessibility/ax_node.h"

class AomAxContentNode : public blink::PlatformAXNode {
 public:
  explicit AomAxContentNode(ui::AXNode);

  const std::string& GetStringAttribute(blink::PlatformAXStringAttribute);

 private:
  ui::AXNode node_;
  DISALLOW_COPY_AND_ASSIGN(AomAxContentNode);
};

#endif  // CONTENT_RENDERER_ACCESSIBILITY_AOM_CONTENT_AX_NODE_H_
