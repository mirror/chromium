// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/accessibility/aom_ax_tree_provider.h"

#include "base/logging.h"
#include "content/renderer/render_frame_impl.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"

// static
AomAxTreeProvider* AomAxTreeProvider::Create() {
  return new AomAxTreeProvider();
}

AomAxTreeProvider::AomAxTreeProvider() {}

AomAxTreeProvider::~AomAxTreeProvider() {}

void AomAxTreeProvider::RequestTreeSnapshot() {
  auto* web_frame = blink::WebLocalFrame::FrameForCurrentContext();
  content::RenderFrameImpl* render_frame =
      content::RenderFrameImpl::FromWebFrame(web_frame);
  content::RenderAccessibilityImpl::SnapshotAccessibilityTree(render_frame,
                                                              &tree_update_);
}
