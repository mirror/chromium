// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/android/synthetic_event_helper.h"

#include "content/browser/renderer_host/render_widget_host_view_android.h"

namespace content {

SyntheticEventHelper::SyntheticEventHelper(WebContents* web_contents)
    : RenderWidgetHostConnector(web_contents), web_contents_(web_contents) {}

WebContents* SyntheticEventHelper::GetWebContents() const {
  return web_contents_;
}

void SyntheticEventHelper::UpdateRenderProcessConnection(
    RenderWidgetHostViewAndroid* old_rwhva,
    RenderWidgetHostViewAndroid* new_rwhva) {
  if (old_rwhva)
    old_rwhva->set_synthetic_event_helper(nullptr);
  if (new_rwhva)
    new_rwhva->set_synthetic_event_helper(this);
}

}  // namespace content
