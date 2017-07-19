// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/android/synthetic_gesture_target_helper.h"

#include "content/browser/renderer_host/render_widget_host_impl.h"
#include "content/browser/web_contents/web_contents_android.h"
#include "content/browser/web_contents/web_contents_impl.h"

namespace content {

SyntheticGestureTargetAndroid* CreateSyntheticGestureTargetAndroid(
    RenderWidgetHostImpl* host) {
  auto* web_contents =
      static_cast<WebContentsImpl*>(host->delegate()->GetAsWebContents());
  return web_contents->GetWebContentsAndroid()->CreateSyntheticGestureTarget(
      host);
}

}  // namespace content
