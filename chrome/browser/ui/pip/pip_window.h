// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_PIP_PIP_WINDOW_H_
#define CHROME_BROWSER_UI_PIP_PIP_WINDOW_H_

#include "ui/base/base_window.h"
#include "ui/compositor/layer.h"

class PipWindow : public ui::BaseWindow {
 public:
  PipWindow() {}
  virtual ~PipWindow() {}

  // Returns a created PipWindow. This is defined in the platform-specific
  // implementation for the class.
  static std::unique_ptr<PipWindow> Create();

  virtual void Init() = 0;
  virtual ui::Layer* GetLayer() = 0;

  // ui::BaseWindow:
  bool IsActive() const override;
  bool IsMaximized() const override;
  bool IsMinimized() const override;
  bool IsFullscreen() const override;
  gfx::NativeWindow GetNativeWindow() const override;
  gfx::Rect GetRestoredBounds() const override;
  ui::WindowShowState GetRestoredState() const override;
  gfx::Rect GetBounds() const override;
  void Show() override;
  void Hide() override;
  void ShowInactive() override;
  void Close() override;
  void Activate() override;
  void Deactivate() override;
  void Maximize() override;
  void Minimize() override;
  void Restore() override;
  void SetBounds(const gfx::Rect& bounds) override;
  void FlashFrame(bool flash) override;
  bool IsAlwaysOnTop() const override;
  void SetAlwaysOnTop(bool always_on_top) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(PipWindow);
};

#endif  // CHROME_BROWSER_UI_PIP_PIP_WINDOW_H_
