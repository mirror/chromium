// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/renderer/aom_ax_tree_provider.h"

#include "base/logging.h"
#include "content/renderer/render_frame_impl.h"
#include "third_party/WebKit/Source/core/frame/WebLocalFrameImpl.h"

// static
AomAxTreeProvider* AomAxTreeProvider::Create() {
  return new AomAxTreeProvider();
}

AomAxTreeProvider::AomAxTreeProvider() {}

AomAxTreeProvider::~AomAxTreeProvider() {}

void AomAxTreeProvider::RequestTreeSnapshot() {
  LOG(ERROR) << "Requesting snapshot";
  auto* web_frame = blink::WebLocalFrame::FrameForCurrentContext();
  content::RenderFrameImpl* render_frame =
      content::RenderFrameImpl::FromWebFrame(web_frame);
  content::RenderAccessibilityImpl::SnapshotAccessibilityTree(render_frame,
                                                              &tree_update_);
}

void AomAxTreeProvider::Walk() {
  LOG(ERROR) << "Starting tree walk";
  LOG(ERROR) << tree_update_.ToString();
}
