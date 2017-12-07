// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/exo/shell_surface.h"

#include "ash/public/cpp/shell_window_ids.h"
#include "ash/public/cpp/window_state_type.h"
#include "ash/wm/window_resizer.h"
#include "ash/wm/window_state.h"
#include "base/logging.h"
#include "base/strings/utf_string_conversions.h"
#include "components/exo/wm_helper.h"
#include "ui/aura/client/cursor_client.h"
#include "ui/aura/window.h"
#include "ui/views/widget/widget.h"
#include "ui/wm/core/window_util.h"

namespace exo {

////////////////////////////////////////////////////////////////////////////////
// ShellSurface, public:

ShellSurface::ShellSurface(Surface* surface,
                           BoundsMode bounds_mode,
                           const gfx::Point& origin,
                           bool activatable,
                           bool can_minimize,
                           int container)
    : ShellSurfaceBase(surface,
                       bounds_mode,
                       origin,
                       activatable,
                       can_minimize,
                       container) {}

ShellSurface::ShellSurface(Surface* surface)
    : ShellSurfaceBase(surface,
                       BoundsMode::SHELL,
                       gfx::Point(),
                       true,
                       true,
                       ash::kShellWindowId_DefaultContainer) {}

ShellSurface::~ShellSurface() {
}

void ShellSurface::SetParent(ShellSurface* parent) {
  TRACE_EVENT1("exo", "ShellSurface::SetParent", "parent",
               parent ? base::UTF16ToASCII(parent->title_) : "null");

  SetParentWindow(parent ? parent->GetWidget()->GetNativeWindow() : nullptr);
}

void ShellSurface::Maximize() {
  TRACE_EVENT0("exo", "ShellSurface::Maximize");

  if (!widget_)
    CreateShellSurfaceWidget(ui::SHOW_STATE_MAXIMIZED);

  // Note: This will ask client to configure its surface even if already
  // maximized.
  ScopedConfigure scoped_configure(this, true);
  widget_->Maximize();
}

void ShellSurface::Minimize() {
  TRACE_EVENT0("exo", "ShellSurface::Minimize");

  if (!widget_)
    CreateShellSurfaceWidget(ui::SHOW_STATE_MINIMIZED);

  // Note: This will ask client to configure its surface even if already
  // minimized.
  ScopedConfigure scoped_configure(this, true);
  widget_->Minimize();
}

void ShellSurface::Restore() {
  TRACE_EVENT0("exo", "ShellSurface::Restore");

  if (!widget_)
    return;

  // Note: This will ask client to configure its surface even if not already
  // maximized or minimized.
  ScopedConfigure scoped_configure(this, true);
  widget_->Restore();
}

void ShellSurface::SetFullscreen(bool fullscreen) {
  TRACE_EVENT1("exo", "ShellSurface::SetFullscreen", "fullscreen", fullscreen);

  if (!widget_)
    CreateShellSurfaceWidget(ui::SHOW_STATE_FULLSCREEN);

  // Note: This will ask client to configure its surface even if fullscreen
  // state doesn't change.
  ScopedConfigure scoped_configure(this, true);
  widget_->SetFullscreen(fullscreen);
}

void ShellSurface::Move() {
  TRACE_EVENT0("exo", "ShellSurface::Move");

  if (!widget_)
    return;

  switch (bounds_mode_) {
    case BoundsMode::SHELL:
      AttemptToStartDrag(HTCAPTION);
      return;
    case BoundsMode::FIXED:
      return;
    case BoundsMode::CLIENT:
      break;
  }

  NOTREACHED();
}

void ShellSurface::Resize(int component) {
  TRACE_EVENT1("exo", "ShellSurface::Resize", "component", component);

  if (!widget_)
    return;

  switch (bounds_mode_) {
    case BoundsMode::SHELL:
      AttemptToStartDrag(component);
      return;
    case BoundsMode::CLIENT:
    case BoundsMode::FIXED:
      return;
  }

  NOTREACHED();
}

void ShellSurface::AttemptToStartDrag(int component) {
  DCHECK(widget_);

  // Cannot start another drag if one is already taking place.
  if (resizer_)
    return;

  aura::Window* window = GetDragWindow();
  if (!window || window->HasCapture())
    return;

  DCHECK_EQ(bounds_mode_, BoundsMode::SHELL);

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

void ShellSurface::InitializeWindowState(ash::wm::WindowState* window_state) {
  // Allow the client to request bounds that do not fill the entire work area
  // when maximized, or the entire display when fullscreen.
  window_state->set_allow_set_bounds_direct(false);
  // Disable movement if bounds are controlled by the client or fixed.
  bool movement_disabled = bounds_mode_ != BoundsMode::SHELL;
  widget_->set_movement_disabled(movement_disabled);
  window_state->set_ignore_keyboard_bounds_change(movement_disabled);
}

}  // namespace exo
