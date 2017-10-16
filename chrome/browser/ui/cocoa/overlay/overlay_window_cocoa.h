// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_OVERLAY_OVERLAY_WINDOW_COCOA_H_
#define CHROME_BROWSER_UI_COCOA_OVERLAY_OVERLAY_WINDOW_COCOA_H_

#include "chrome/browser/ui/overlay/overlay_window.h"

@class NSWindowController;

// The cocoa implementation of OverlayWindow.
class OverlayWindowCocoa : public OverlayWindow {
 public:
  OverlayWindowCocoa();
  ~OverlayWindowCocoa() override;

  // OverlayWindow:
  void Init() override;
  bool IsActive() const override;
  void Show() override;
  void Hide() override;
  void Close() override;
  void Activate() override;
  bool IsAlwaysOnTop() const override;
  ui::Layer* GetLayer() override;
  gfx::NativeWindow GetNativeWindow() const override;
  gfx::Rect GetBounds() const override;

 private:
  NSWindow* window_;
  NSWindowController* controller_;

  DISALLOW_COPY_AND_ASSIGN(OverlayWindowCocoa);
};

#endif  // CHROME_BROWSER_UI_COCOA_OVERLAY_OVERLAY_WINDOW_COCOA_H_