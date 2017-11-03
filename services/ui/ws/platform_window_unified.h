// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_WS_PLATFORM_WINDOW_UNIFIED_H_
#define SERVICES_UI_WS_PLATFORM_WINDOW_UNIFIED_H_

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/platform_window/platform_window.h"

namespace ui {

class PlatformWindowDelegate;

class PlatformWindowUnified : public PlatformWindow {
 public:
  explicit PlatformWindowUnified(PlatformWindowDelegate* delegate);
  ~PlatformWindowUnified() override;

 protected:
  PlatformWindowDelegate* delegate() { return delegate_; }

 private:
  // PlatformWindow:
  void Show() override;
  void Hide() override;
  void Close() override;
  void PrepareForShutdown() override;
  void SetBounds(const gfx::Rect& bounds) override;
  gfx::Rect GetBounds() override;
  void SetTitle(const base::string16& title) override;
  void SetCapture() override;
  void ReleaseCapture() override;
  void ToggleFullscreen() override;
  void Maximize() override;
  void Minimize() override;
  void Restore() override;
  void SetCursor(PlatformCursor cursor) override;
  void MoveCursorTo(const gfx::Point& location) override;
  void ConfineCursorToBounds(const gfx::Rect& bounds) override;
  PlatformImeController* GetPlatformImeController() override;

  PlatformWindowDelegate* delegate_;
  gfx::Rect bounds_;

  DISALLOW_COPY_AND_ASSIGN(PlatformWindowUnified);
};

}  // namespace ui

#endif  // SERVICES_UI_WS_PLATFORM_WINDOW_UNIFIED_H_
