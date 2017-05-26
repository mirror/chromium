// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_PIP_PIP_WINDOW_VIEWS_H_
#define CHROME_BROWSER_UI_VIEWS_PIP_PIP_WINDOW_VIEWS_H_

#include "chrome/browser/ui/pip/pip_window.h"
#include "ui/views/widget/widget.h"

// The Views window for Picture in Picture.
class PipWindowViews : public PipWindow {
 public:
  PipWindowViews();
  ~PipWindowViews() override;

  void Init() override;
  ui::Layer* GetLayer() override;

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
  views::Widget* widget_;

  DISALLOW_COPY_AND_ASSIGN(PipWindowViews);
};

#endif  // CHROME_BROWSER_UI_VIEWS_PIP_PIP_WINDOW_VIEWS_H_
