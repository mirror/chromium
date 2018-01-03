// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_RENDERER_AOM_AX_TREE_PROVIDER_H_
#define CHROME_RENDERER_AOM_AX_TREE_PROVIDER_H_

#include "third_party/WebKit/public/platform/PlatformAXTree.h"

#include "base/macros.h"
#include "content/common/ax_content_node_data.h"
#include "content/renderer/accessibility/render_accessibility_impl.h"

class AomAxTreeProvider : public blink::PlatformAXTree {
 public:
  static AomAxTreeProvider* Create();

  ~AomAxTreeProvider();

  // blink::PlatformAXTree implementation.
  void RequestTreeSnapshot() override;
  void Walk() override;

 private:
  AomAxTreeProvider();
  content::AXContentTreeUpdate tree_update_;
  DISALLOW_COPY_AND_ASSIGN(AomAxTreeProvider);
};

#endif  // CHROME_RENDERER_AOM_AX_TREE_PROVIDER_H_
