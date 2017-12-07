// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/render_widget_host_view_desktop_base.h"

#include "base/debug/stack_trace.h"
#include "content/browser/renderer_host/render_widget_host_delegate.h"
#include "content/browser/renderer_host/render_widget_host_impl.h"

namespace content {

RenderWidgetHostViewDesktopBase::~RenderWidgetHostViewDesktopBase() = default;
RenderWidgetHostViewDesktopBase::RenderWidgetHostViewDesktopBase() = default;

void RenderWidgetHostViewDesktopBase::Hide() {
  LOG(ERROR) << "Hide";
  base::debug::StackTrace().Print();
  DoHide();
  DCHECK_EQ(Visibility::HIDDEN, GetVisibility());

  // If the view was already hidden, WasHidden() might not have been called.
  // Call WasHidden() explicitly to make sure that the host is always hidden
  // when Hide() is called (it might not if it was constructed with |hidden| =
  // false).
  RenderWidgetHostImpl* host = GetRenderWidgetHostImpl();
  if (host && !host->is_hidden() && !IsCaptured())
    WasHidden();
}

void RenderWidgetHostViewDesktopBase::ForceShow() {
  Show();
  WasShown();
}

void RenderWidgetHostViewDesktopBase::VisibilityChanged() {
  VisibilityOrCaptureStateChanged();
}

void RenderWidgetHostViewDesktopBase::CaptureStateChanged() {
  VisibilityOrCaptureStateChanged();
}

void RenderWidgetHostViewDesktopBase::VisibilityOrCaptureStateChanged() {
  const Visibility visibility = GetVisibility();
  if (visibility == Visibility::VISIBLE)
    was_ever_visible_ = true;

  if (visibility == Visibility::VISIBLE || IsCaptured()) {
    WasShown();
  } else {
    // If the view was never visible, trust that its initial visibility is
    // correct and don't call WasHidden(). It is important not to call
    // WasHidden() in these situations that may trigger VisibilityChanged():
    // - The view is added to a hidden parent that is about to be shown.
    // - The view is added to a hidden parent but is still expected to act
    //   like if it was visible (e.g. prerendering).
    if (was_ever_visible_)
      WasHidden();
  }
}

bool RenderWidgetHostViewDesktopBase::IsCaptured() const {
  RenderWidgetHostImpl* const host = GetRenderWidgetHostImpl();
  return host && host->IsCaptured();
}

}  // namespace content
