// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/render_widget_host_view_desktop_base.h"

#include "content/browser/renderer_host/render_widget_host_delegate.h"
#include "content/browser/renderer_host/render_widget_host_impl.h"

namespace content {

RenderWidgetHostViewDesktopBase::~RenderWidgetHostViewDesktopBase() = default;
RenderWidgetHostViewDesktopBase::RenderWidgetHostViewDesktopBase() = default;

void RenderWidgetHostViewDesktopBase::VisibilityChanged() {
  VisibilityOrCaptureStateChanged();
}

void RenderWidgetHostViewDesktopBase::CaptureStateChanged() {
  VisibilityOrCaptureStateChanged();
}

void RenderWidgetHostViewDesktopBase::VisibilityOrCaptureStateChanged() {
  RenderWidgetHostImpl* host = GetRenderWidgetHostImpl();
  const Visibility visibility = GetVisibility();
  const bool is_visible_or_captured =
      (visibility == Visibility::VISIBLE) ||
      (host && host->delegate() && host->delegate()->IsCaptured());

  if (is_visible_or_captured) {
    was_shown_once_ = true;
    WasShown();
  } else {
    // If the view was never shown, trust that it has an appropriate initial
    // visibility and don't call WasHidden() (this can be called when a view
    // that was never shown is added to a non-visible parent that is about to
    // become visible).
    if (was_shown_once_)
      WasHidden();
  }
}

}  // namespace content
