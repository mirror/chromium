// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/render_widget_host_view_desktop_base.h"

#include "content/browser/renderer_host/render_widget_host_delegate.h"
#include "content/browser/renderer_host/render_widget_host_impl.h"

namespace content {

RenderWidgetHostViewDesktopBase::~RenderWidgetHostViewDesktopBase() = default;
RenderWidgetHostViewDesktopBase::RenderWidgetHostViewDesktopBase() = default;

void RenderWidgetHostViewDesktopBase::Hide() {
  DoHide();
  DCHECK_EQ(Visibility::HIDDEN, GetVisibility());

  // If the view was already hidden, VisibilityChanged() might not have been
  // called. Call WasHidden() directly to make sure that the host is hidden (it
  // might not if it was created with |hidden| = false).
  RenderWidgetHostImpl* host = GetRenderWidgetHostImpl();
  if (host && !host->is_hidden() && !IsCaptured())
    WasHidden();
}

void RenderWidgetHostViewDesktopBase::VisibilityChanged() {
  VisibilityOrCaptureStateChanged();
}

void RenderWidgetHostViewDesktopBase::CaptureStateChanged() {
  VisibilityOrCaptureStateChanged();
}

void RenderWidgetHostViewDesktopBase::VisibilityOrCaptureStateChanged() {
  if (GetVisibility() == Visibility::VISIBLE || IsCaptured()) {
    can_hide_ = true;
    WasShown();
  } else if (can_hide_) {
    WasHidden();
  }
}

bool RenderWidgetHostViewDesktopBase::IsCaptured() const {
  RenderWidgetHostImpl* const host = GetRenderWidgetHostImpl();
  return host && host->delegate() && host->delegate()->IsCaptured();
}

}  // namespace content
