// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_RENDER_WIDGET_HOST_VIEW_BASE_UNITTEST_H_
#define CONTENT_BROWSER_RENDERER_HOST_RENDER_WIDGET_HOST_VIEW_BASE_UNITTEST_H_

#include "base/callback_forward.h"

namespace content {

class MockRenderWidgetHostDelegate;
class RenderWidgetHostViewBase;

using SetIsOccludedCallback = base::RepeatingCallback<void(bool is_occluded)>;

// Tests for implementations of RenderWidgetHostViewBase.
void RenderWidgetHostViewBase_TestShowHide(RenderWidgetHostViewBase* view);
void RenderWidgetHostViewBase_TestShowHideAndCapture(
    RenderWidgetHostViewBase* view,
    MockRenderWidgetHostDelegate* delegate);
void RenderWidgetHostViewBase_TestOcclusion(
    RenderWidgetHostViewBase* view,
    const SetIsOccludedCallback& set_is_occluded);
void RenderWidgetHostViewBase_TestOcclusionAndCapture(
    RenderWidgetHostViewBase* view,
    MockRenderWidgetHostDelegate* delegate,
    const SetIsOccludedCallback& set_is_occluded);

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_RENDER_WIDGET_HOST_VIEW_BASE_UNITTEST_H_
