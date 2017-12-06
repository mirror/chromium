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

 protected:
  RenderWidgetHostViewDesktopBase();

 private:
  DISALLOW_COPY_AND_ASSIGN(RenderWidgetHostViewDesktopBase);
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_RENDER_WIDGET_HOST_VIEW_DESKTOP_BASE_H_
