// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_RENDER_WIDGET_HOST_VIEW_DESKTOP_BASE_H_
#define CONTENT_BROWSER_RENDERER_HOST_RENDER_WIDGET_HOST_VIEW_DESKTOP_BASE_H_

#include "base/macros.h"
#include "content/browser/renderer_host/render_widget_host_view_base.h"
#include "content/common/content_export.h"

namespace content {

// Base implementation for RenderWidgetHostViewBase subclasses that support
// occlusion tracking.
class CONTENT_EXPORT RenderWidgetHostViewDesktopBase
    : public RenderWidgetHostViewBase {
 public:
  ~RenderWidgetHostViewDesktopBase() override;

  // Implementations must call this when the visibility of the view changes.
  // Calls WasShown()/WasHidden() depending on the visibility and capture state
  // of the view.
  void VisibilityChanged();

 protected:
  RenderWidgetHostViewDesktopBase();

 private:
  // Invoked when the view becomes visible and/or captured. Should enable
  // rendering and adjust the priority of the process hosting this view.
  virtual void WasShown() = 0;

  // Invoked when the view becomes non-visible and non-captured. Should disable
  // rendering and adjust the priority of the process hosting this view.
  virtual void WasHidden() = 0;

  // RenderWidgetHostViewBase:
  void CaptureStateChanged() final;

  // Calls WasShown()/WasHidden() depending on the visibility and capture state
  // of the view.
  void VisibilityOrCaptureStateChanged();

  // Whether the view was shown at least once.
  bool was_shown_once_ = false;

  DISALLOW_COPY_AND_ASSIGN(RenderWidgetHostViewDesktopBase);
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_RENDER_WIDGET_HOST_VIEW_DESKTOP_BASE_H_
