// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/exo/xdg_shell_surface.h"

#include <algorithm>

#include "ash/frame/custom_frame_view_ash.h"
#include "ash/public/cpp/shelf_types.h"
#include "ash/public/cpp/shell_window_ids.h"
#include "ash/public/cpp/window_properties.h"
#include "ash/public/cpp/window_state_type.h"
#include "ash/public/interfaces/window_pin_type.mojom.h"
#include "ash/wm/drag_window_resizer.h"
#include "ash/wm/window_resizer.h"
#include "ash/wm/window_state.h"
#include "ash/wm/window_util.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/trace_event/trace_event.h"
#include "base/trace_event/trace_event_argument.h"
#include "cc/trees/layer_tree_frame_sink.h"
#include "components/exo/surface.h"
#include "components/exo/wm_helper.h"
#include "services/ui/public/interfaces/window_tree_constants.mojom.h"
#include "ui/aura/client/aura_constants.h"
#include "ui/aura/client/cursor_client.h"
#include "ui/aura/window.h"
#include "ui/aura/window_event_dispatcher.h"
#include "ui/aura/window_targeter.h"
#include "ui/aura/window_tree_host.h"
#include "ui/base/accelerators/accelerator.h"
#include "ui/base/class_property.h"
#include "ui/compositor/compositor.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/gfx/path.h"
#include "ui/views/widget/widget.h"
#include "ui/wm/core/coordinate_conversion.h"
#include "ui/wm/core/shadow.h"
#include "ui/wm/core/shadow_controller.h"
#include "ui/wm/core/shadow_types.h"
#include "ui/wm/core/window_animations.h"
#include "ui/wm/core/window_util.h"

namespace exo {

////////////////////////////////////////////////////////////////////////////////
// XdgShellSurface, public:

XdgShellSurface::XdgShellSurface(Surface* surface,
                                 BoundsMode bounds_mode,
                                 const gfx::Point& origin,
                                 bool activatable,
                                 bool can_minimize,
                                 int container)
    : ShellSurface(surface,
                   bounds_mode,
                   origin,
                   activatable,
                   can_minimize,
                   container) {}

XdgShellSurface::XdgShellSurface(Surface* surface)
    : ShellSurface(surface,
                   BoundsMode::SHELL,
                   gfx::Point(),
                   true,
                   true,
                   ash::kShellWindowId_DefaultContainer) {}

XdgShellSurface::~XdgShellSurface() {
  if (parent_)
    parent_->RemoveObserver(this);
  if (widget_)
    widget_->GetNativeWindow()->RemoveObserver(this);
}

void XdgShellSurface::Move() {
  TRACE_EVENT0("exo", "ShellSurface::Move");
}

void XdgShellSurface::InitializeWindowState(
    ash::wm::WindowState* window_state) {
  // Allow the client to request bounds that do not fill the entire work area
  // when maximized, or the entire display when fullscreen.
  window_state->set_allow_set_bounds_direct(false);
  // Disable movement if bounds are controlled by the client or fixed.
  bool movement_disabled = bounds_mode_ != BoundsMode::SHELL;
  widget_->set_movement_disabled(movement_disabled);
  window_state->set_ignore_keyboard_bounds_change(movement_disabled);
}

void XdgShellSurface::Resize(int component) {
  TRACE_EVENT1("exo", "ShellSurface::Resize", "component", component);

  if (!widget_)
    return;

  switch (bounds_mode_) {
    case BoundsMode::SHELL:
      AttemptToStartDrag(component);
      return;
    case BoundsMode::FIXED:
      return;
    case BoundsMode::CLIENT:
      break;
  }

  NOTREACHED();
}

void XdgShellSurface::SetBoundsMode(BoundsMode mode) {
  TRACE_EVENT1("exo", "ShellSurface::SetBoundsMode", "mode",
               static_cast<int>(mode));

  bounds_mode_ = mode;
}

void XdgShellSurface::SetCanMinimize(bool can_minimize) {
  TRACE_EVENT1("exo", "ShellSurface::SetCanMinimize", "can_minimize",
               static_cast<int>(can_minimize));
  can_minimize_ = can_minimize;
}

////////////////////////////////////////////////////////////////////////////////
// views::WidgetDelegate overrides:

void XdgShellSurface::SaveWindowPlacement(const gfx::Rect& bounds,
                                          ui::WindowShowState show_state) {
  WidgetDelegate::SaveWindowPlacement(bounds, show_state);
}

bool XdgShellSurface::GetSavedWindowPlacement(
    const views::Widget* widget,
    gfx::Rect* bounds,
    ui::WindowShowState* show_state) const {
  return WidgetDelegate::GetSavedWindowPlacement(widget, bounds, show_state);
}

////////////////////////////////////////////////////////////////////////////////
// aura::WindowObserver overrides:

void XdgShellSurface::OnWindowBoundsChanged(aura::Window* window,
                                            const gfx::Rect& old_bounds,
                                            const gfx::Rect& new_bounds,
                                            ui::PropertyChangeReason reason) {
  if (!widget_ || !root_surface() || ignore_window_bounds_changes_)
    return;

  if (window == widget_->GetNativeWindow()) {
    if (new_bounds.size() == old_bounds.size())
      return;

    // If size changed then give the client a chance to produce new contents
    // before origin on screen is changed. Retain the old origin by reverting
    // the origin delta until the next configure is acknowledged.
    gfx::Vector2d delta = new_bounds.origin() - old_bounds.origin();
    origin_offset_ -= delta;
    pending_origin_offset_accumulator_ += delta;

    UpdateSurfaceBounds();

    // The shadow size may be updated to match the widget. Change it back
    // to the shadow content size.
    // TODO(oshima): When the window reiszing is enabled, we may want to
    // implement shadow management here instead of using shadow controller.
    UpdateShadow();

    Configure();
  }
}

////////////////////////////////////////////////////////////////////////////////
// ShellSurface, private:

aura::Window* XdgShellSurface::GetDragWindow() {
  switch (bounds_mode_) {
    case BoundsMode::SHELL:
      return widget_->GetNativeWindow();

    case BoundsMode::FIXED:
      return nullptr;
    case BoundsMode::CLIENT:
      break;
  }

  NOTREACHED();
  return nullptr;
}

void XdgShellSurface::AttemptToStartDrag(int component) {
  DCHECK(widget_);

  // Cannot start another drag if one is already taking place.
  if (resizer_)
    return;

  aura::Window* window = GetDragWindow();
  if (!window || window->HasCapture())
    return;

  DCHECK_EQ(BoundsMode::SHELL, bounds_mode_);
  // Set the cursor before calling CreateWindowResizer(), as that will
  // eventually call LockCursor() and prevent the cursor from changing.
  aura::client::CursorClient* cursor_client =
      aura::client::GetCursorClient(window->GetRootWindow());
  if (!cursor_client)
    return;

  switch (component) {
    case HTCAPTION:
      cursor_client->SetCursor(ui::CursorType::kPointer);
      break;
    case HTTOP:
      cursor_client->SetCursor(ui::CursorType::kNorthResize);
      break;
    case HTTOPRIGHT:
      cursor_client->SetCursor(ui::CursorType::kNorthEastResize);
      break;
    case HTRIGHT:
      cursor_client->SetCursor(ui::CursorType::kEastResize);
      break;
    case HTBOTTOMRIGHT:
      cursor_client->SetCursor(ui::CursorType::kSouthEastResize);
      break;
    case HTBOTTOM:
      cursor_client->SetCursor(ui::CursorType::kSouthResize);
      break;
    case HTBOTTOMLEFT:
      cursor_client->SetCursor(ui::CursorType::kSouthWestResize);
      break;
    case HTLEFT:
      cursor_client->SetCursor(ui::CursorType::kWestResize);
      break;
    case HTTOPLEFT:
      cursor_client->SetCursor(ui::CursorType::kNorthWestResize);
      break;
    default:
      NOTREACHED();
      break;
  }

  resizer_ = ash::CreateWindowResizer(window, GetMouseLocation(), component,
                                      wm::WINDOW_MOVE_SOURCE_MOUSE);
  if (!resizer_)
    return;

  // Apply pending origin offsets and resize direction before starting a
  // new resize operation. These can still be pending if the client has
  // acknowledged the configure request but not yet called Commit().
  origin_offset_ += pending_origin_offset_;
  pending_origin_offset_ = gfx::Vector2d();
  resize_component_ = pending_resize_component_;

  WMHelper::GetInstance()->AddPreTargetHandler(this);
  window->SetCapture();

  // Notify client that resizing state has changed.
  if (IsResizing())
    Configure();
}

}  // namespace exo
