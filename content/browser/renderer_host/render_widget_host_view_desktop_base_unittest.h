// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_RENDER_host_VIEW_DESKTOP_BASE_H_
#define CONTENT_BROWSER_RENDERER_HOST_RENDER_host_VIEW_DESKTOP_BASE_H_

#include "base/callback_forward.h"

namespace content {

class MockRenderWidgetHostDelegate;
class RenderWidgetHostViewDesktopBase;

using SetIsOccludedCallback = base::Callback<void(bool is_occluded)>;

// Tests for derived classes of RenderWidgetHostViewDesktopBase.
void RenderWidgetHostViewDesktopBase_TestShowHide(
    RenderWidgetHostViewDesktopBase* view);
void RenderWidgetHostViewDesktopBase_TestShowHideAndCapture(
    RenderWidgetHostViewDesktopBase* view,
    MockRenderWidgetHostDelegate* delegate);
void RenderWidgetHostViewDesktopBase_TestOcclusion(
    RenderWidgetHostViewDesktopBase* view,
    const SetIsOccludedCallback& set_is_occluded);
void RenderWidgetHostViewDesktopBase_TestOcclusionAndCapture(
    RenderWidgetHostViewDesktopBase* view,
    MockRenderWidgetHostDelegate* delegate,
    const SetIsOccludedCallback& set_is_occluded);

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_RENDER_host_VIEW_DESKTOP_BASE_H_
