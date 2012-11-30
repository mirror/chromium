// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_AURA_ROOT_WINDOW_HOST_DRM_H_
#define UI_AURA_ROOT_WINDOW_HOST_DRM_H_
#pragma once

#include "ui/aura/root_window_host.h"

#include "base/message_loop.h"
#include "base/message_pump_dispatcher.h"
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
                          public MessageLoop::Dispatcher {
 public:
  explicit RootWindowHostDRM(const gfx::Rect& bounds);
  virtual ~RootWindowHostDRM();

  static int GetDRMFd();
  static void SetDRMFd(int fd);

  // Overridden from Dispatcher overrides:
  virtual bool Dispatch(const base::NativeEvent& event) OVERRIDE;

  RootWindowHostDelegate* delegate() { return delegate_; }
  void SetDelegate(RootWindowHostDelegate* delegate) { delegate_ = delegate; }

 private:
  // RootWindowHost Overrides.
  virtual RootWindow* GetRootWindow() OVERRIDE;
  virtual gfx::AcceleratedWidget GetAcceleratedWidget() OVERRIDE;
  virtual gfx::Rect GetBounds() const OVERRIDE;
  virtual void SetBounds(const gfx::Rect& bounds) OVERRIDE;
  virtual gfx::Size GetSize() const OVERRIDE;
  virtual void SetSize(const gfx::Size& size) OVERRIDE;
  virtual gfx::Point GetLocationOnNativeScreen() const OVERRIDE;
  virtual void SetCursor(gfx::NativeCursor cursor_type) OVERRIDE;
  virtual void ShowCursor(bool show) OVERRIDE;
  virtual bool QueryMouseLocation(gfx::Point*) OVERRIDE;
  virtual void MoveCursorTo(const gfx::Point& location) OVERRIDE;
  virtual void PostNativeEvent(const base::NativeEvent& event) OVERRIDE;
  virtual void OnDeviceScaleFactorChanged(float device_scale_factor) OVERRIDE;
  virtual void Show() OVERRIDE;
  virtual void Hide() OVERRIDE;

  // Nothing to do on DRM
  virtual void ToggleFullScreen() {};
  virtual void SetCapture() {};
  virtual void ReleaseCapture() {};
  virtual bool ConfineCursorToRootWindow() { return true; };
  virtual void UnConfineCursor() {};
  virtual void SetFocusWhenShown(bool focus_when_shown) {};
  virtual bool GrabSnapshot(
      const gfx::Rect& snapshot_bounds,
      std::vector<unsigned char>* png_representation) { return true; };
  virtual bool CopyAreaToSkCanvas(const gfx::Rect& source_bounds,
                                  const gfx::Point& dest_offset,
                                  SkCanvas* canvas) { return true}
  virtual void PrepareForShutdown() {}

  // RootWindowHostDRM private functions
  bool SetupKMS();
  xkb_desc *CreateXKB();

  // Looks up and returns the cursor image from the image library.
  ui::PlatformCursor GetPlatformCursor(gfx::NativeCursor);

  // Sets the current DRM cursor to |cursor|.  Does not check or update
  // |current_cursor_|.
  void SetCursorInternal(gfx::NativeCursor cursor);

  // Sets the DRM cursor to the current location.
  void UpdateDRMCursor();

  RootWindowHostDelegate* delegate_;

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
  ui::PlatformCursor current_platform_cursor_;

  class ImageCursors;
  scoped_ptr<ImageCursors> image_cursors_;

  // For hiding the cursor.
  ui::Cursor empty_cursor_;

  // The default cursor is showed after startup, and hidden when touch pressed.
  // Once mouse moved, the cursor is immediately displayed.
  bool is_cursor_visible_;

  // The bounds of the window.
  gfx::Rect bounds_;

  DISALLOW_COPY_AND_ASSIGN(RootWindowHostDRM);
};

}  // namespace aura

#endif  // UI_AURA_ROOT_WINDOW_HOST_DRM_H_
