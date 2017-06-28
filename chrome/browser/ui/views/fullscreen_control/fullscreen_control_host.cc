// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/fullscreen_control/fullscreen_control_host.h"

#include "base/i18n/rtl.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/fullscreen_control/fullscreen_control_view.h"
#include "ui/events/event.h"
#include "ui/events/event_constants.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/views/view.h"

FullscreenControlHost::FullscreenControlHost(BrowserView* browser_view,
                                             views::View* host_view)
    : DropdownBarHost(browser_view),
      browser_view_(browser_view),
      fullscreen_control_view_(new FullscreenControlView(browser_view, this)) {
  Init(host_view, fullscreen_control_view_, fullscreen_control_view_);
}

FullscreenControlHost::~FullscreenControlHost() = default;

bool FullscreenControlHost::AcceleratorPressed(
    const ui::Accelerator& accelerator) {
  return false;
}

bool FullscreenControlHost::CanHandleAccelerators() const {
  return false;
}

gfx::Rect FullscreenControlHost::GetDialogPosition(
    gfx::Rect avoid_overlapping_rect) {
  gfx::Rect widget_bounds;
  GetWidgetBounds(&widget_bounds);
  if (widget_bounds.IsEmpty())
    return gfx::Rect();

  // Ask the view how large an area it needs to draw on.
  gfx::Size prefsize = view()->GetPreferredSize();

  if (widget_bounds.width() < prefsize.width())
    prefsize.set_width(widget_bounds.width());

  // Place the view in the top right corner of the widget boundaries (top left
  // for RTL languages).
  gfx::Rect view_location;
  int x = widget_bounds.x();
  if (!base::i18n::IsRTL())
    x += widget_bounds.width() - prefsize.width();
  int y = widget_bounds.y();
  view_location.SetRect(x, y, prefsize.width(), prefsize.height());

  return view_location;
}

void FullscreenControlHost::OnMouseEvent(ui::MouseEvent* event) {
  if (event->type() == ui::ET_MOUSE_MOVED)
    HandleFullScreenControlVisibility(event);
}

void FullscreenControlHost::OnTouchEvent(ui::TouchEvent* event) {
  if (event->type() == ui::ET_TOUCH_MOVED)
    HandleFullScreenControlVisibility(event);
}

void FullscreenControlHost::OnGestureEvent(ui::GestureEvent* event) {
  if (event->type() == ui::ET_GESTURE_LONG_PRESS) {
    if (IsAnimating())
      return;

    if (browser_view_->IsFullscreen()) {
      if (!IsVisible())
        Show(true);
    }
  }
}

void FullscreenControlHost::HandleFullScreenControlVisibility(
    const ui::LocatedEvent* event) {
  if (IsAnimating())
    return;

  if (browser_view_->IsFullscreen()) {
    if (IsVisible()) {
      float control_height =
          static_cast<float>(view()->GetContentsBounds().height());
      float y_limit = control_height + (control_height / 2.0);
      if (event->y() >= y_limit)
        Hide(true);
    } else {
      if (event->y() <= 3.0)
        Show(true);
    }
  } else if (IsVisible()) {
    Hide(true);
  }
}
