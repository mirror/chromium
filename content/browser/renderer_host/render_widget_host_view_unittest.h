// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_RENDER_WIDGET_HOST_VIEW_UNITTEST_H_
#define CONTENT_BROWSER_RENDERER_HOST_RENDER_WIDGET_HOST_VIEW_UNITTEST_H_

#include "base/callback_forward.h"

namespace content {

class RenderWidgetHostViewBase;

using SetIsOccludedCallback = base::RepeatingCallback<void(bool is_occluded)>;

// Tests for implementations of RenderWidgetHostViewBase.
void RenderWidgetHostView_TestShowHide(RenderWidgetHostViewDesktopBase* view);
void RenderWidgetHostView_TestOcclusion(
    RenderWidgetHostViewDesktopBase* view,
    const SetIsOccludedCallback& set_is_occluded);

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_RENDER_WIDGET_HOST_VIEW_DESKTOP_BASE_UNITTEST_H_
