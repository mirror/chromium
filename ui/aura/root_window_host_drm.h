// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_AURA_ROOT_WINDOW_HOST_DRM_H_
#define UI_AURA_ROOT_WINDOW_HOST_DRM_H_
#pragma once

#include "ui/aura/env.h"
#include "ui/aura/root_window_host.h"

#include "base/message_loop.h"
#include "ui/aura/cursor.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/gfx/rect.h"

struct gbm_device;
struct gbm_bo;
struct _drmModeCrtc;
struct _drmModeConnector;
struct _drmModeEncoder;
struct _drmModeModeInfo;
typedef struct _drmModeCrtc drmModeCrtc;
typedef struct _drmModeConnector drmModeConnector;
typedef struct _drmModeEncoder drmModeEncoder;
typedef struct _drmModeModeInfo drmModeModeInfo;

struct xkb_desc;

namespace aura {

struct kms {
   drmModeConnector *connector;
   drmModeEncoder *encoder;
   drmModeModeInfo *mode;
};

class RootWindowHostDRM : public RootWindowHost,
                          public Dispatcher {
 public:
  explicit RootWindowHostDRM(const gfx::Rect& bounds);
  virtual ~RootWindowHostDRM();
  // Returns a pointer to the default display
  struct gbm_device* GetDisplay();

  static RootWindowHostDRM* GetInstance();

  // Handles an event targeted at this host's window.
  base::MessagePumpDispatcher::DispatchStatus Dispatch(base::NativeEvent ev);

 private:
  // RootWindowHost Overrides.
  virtual void SetRootWindow(RootWindow* root_window) OVERRIDE;
  virtual gfx::AcceleratedWidget GetAcceleratedWidget() OVERRIDE;
  virtual gfx::Rect GetBounds() const OVERRIDE;
  virtual void SetBounds(const gfx::Rect& bounds) OVERRIDE;
  virtual gfx::Size GetSize() const OVERRIDE;
  virtual void SetSize(const gfx::Size& size) OVERRIDE;
  virtual gfx::Point GetLocationOnNativeScreen() const OVERRIDE;
  virtual void SetCapture() {};
  virtual void ReleaseCapture() {};
  virtual void SetCursor(gfx::NativeCursor cursor_type) OVERRIDE;
  virtual void ShowCursor(bool show) OVERRIDE;
  virtual gfx::Point QueryMouseLocation() OVERRIDE;
  virtual bool ConfineCursorToRootWindow() OVERRIDE;
  virtual void UnConfineCursor() OVERRIDE;
  virtual void MoveCursorTo(const gfx::Point& location) OVERRIDE;
  virtual void PostNativeEvent(const base::NativeEvent& event) OVERRIDE;

  // Nothing to do on DRM
  virtual void ToggleFullScreen() {};
  virtual void Show() {};

  // RootWindowHostDRM private functions
  bool SetupKMS();
  xkb_desc *CreateXKB();

  RootWindow* root_window_;

  // The display and the fd for the graphics card.
  struct gbm_device* display_;
  drmModeCrtc *saved_crtc_;
  struct kms kms_;
  struct xkb_desc *xkb_;
  int fd_;
  int num_fbs_;
  int modifiers_;

  // Current Aura cursor.
  gfx::NativeCursor current_cursor_;
  gfx::Point cursor_position_;

  // The default cursor is showed after startup, and hidden when touch pressed.
  // Once mouse moved, the cursor is immediately displayed.
  bool is_cursor_visible_;

  // The bounds of the window.
  gfx::Rect bounds_;

  DISALLOW_COPY_AND_ASSIGN(RootWindowHostDRM);
};

}  // namespace aura

#endif  // UI_AURA_ROOT_WINDOW_HOST_DRM_H_
