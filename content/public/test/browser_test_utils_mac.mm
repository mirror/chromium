// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/test/browser_test_utils.h"

#include "content/browser/renderer_host/render_widget_host_delegate.h"
#include "content/browser/renderer_host/render_widget_host_view_mac.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"

namespace content {

void CopyHack(WebContents* contents) {
  RenderWidgetHostView* view = contents->GetMainFrame()->GetView();
  auto* view_mac = static_cast<RenderWidgetHostViewMac*>(view);
  auto* delegate = view_mac->GetFocusedRenderWidgetHostDelegate();
  if (delegate)
    delegate->Copy();
}

}  // namespace content
