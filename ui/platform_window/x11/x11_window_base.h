// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_PLATFORM_WINDOW_X11_X11_WINDOW_BASE_H_
#define UI_PLATFORM_WINDOW_X11_X11_WINDOW_BASE_H_

#include <stdint.h>

#include <X11/X.h>

#include <set>

#include "base/callback.h"
#include "base/macros.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/x/x11_types.h"
#include "ui/platform_window/platform_window.h"
#include "ui/platform_window/platform_window_delegate.h"
#include "ui/platform_window/x11/x11_window_export.h"

namespace ui {

class XScopedEventSelector;

// Abstract base implementation for a X11 based PlatformWindow. Methods that
// are platform specific are left unimplemented.
class X11_WINDOW_EXPORT X11WindowBase : public PlatformWindow {
 public:
  X11WindowBase(PlatformWindowDelegate* delegate, const gfx::Rect& bounds);
  ~X11WindowBase() override;

  // Creates new underlying XWindow. Does not map XWindow.
  void Create();

  // PlatformWindow:
  void Show() override;
  void Hide() override;
  void Close() override;
  void SetBounds(const gfx::Rect& bounds) override;
  gfx::Rect GetBounds() override;
  void SetTitle(const base::string16& title) override;
  void SetCapture() override;
  void ReleaseCapture() override;
  void ToggleFullscreen() override;
  void Maximize() override;
  void Minimize() override;
  void Restore() override;
  void MoveCursorTo(const gfx::Point& location) override;
  void ConfineCursorToBounds(const gfx::Rect& bounds) override;
  PlatformImeController* GetPlatformImeController() override;

 protected:
  void Destroy();

  PlatformWindowDelegate* delegate() { return delegate_; }
  XDisplay* xdisplay() { return xdisplay_; }
  XID xwindow() const { return xwindow_; }

  // Checks if XEvent is for this XWindow.
  bool IsEventForXWindow(const XEvent& xev) const;

  // Processes events for this XWindow.
  void ProcessXWindowEvent(XEvent* xev);

 private:
  // Sends a message to the x11 window manager, enabling or disabling the
  // states |state1| and |state2|.
  // TODO(msisov, tonikitoo): share this with DesktopWindowTreeHostX11.
  void SetWMSpecState(bool enabled, ::Atom state1, ::Atom state2);

  // Called when WM_STATE property is changed.
  void OnWMStateUpdated();

  // Checks if the window manager has set a specific state.
  // TODO(msisov, tonikitoo): share this with DesktopWindowTreeHostX11.
  bool HasWMSpecProperty(const char* property) const;

  // PlatformWindow overrides:
  bool IsMinimized() const override;
  bool IsMaximized() const override;
  bool IsFullscreen() const override;

  PlatformWindowDelegate* delegate_;

  XDisplay* xdisplay_;
  XID xwindow_;
  XID xroot_window_;
  std::unique_ptr<ui::XScopedEventSelector> xwindow_events_;

  base::string16 window_title_;

  // The bounds of |xwindow_|.
  gfx::Rect bounds_;

  // The window manager state bits.
  std::set<::Atom> window_properties_;

  bool window_mapped_ = false;
  bool is_fullscreen_ = false;

  DISALLOW_COPY_AND_ASSIGN(X11WindowBase);
};

}  // namespace ui

#endif  // UI_PLATFORM_WINDOW_X11_X11_WINDOW_BASE_H_
