// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_OVERLAY_OVERLAY_WINDOW_VIEWS_H_
#define CHROME_BROWSER_UI_VIEWS_OVERLAY_OVERLAY_WINDOW_VIEWS_H_

#include "chrome/browser/ui/overlay/overlay_window.h"

namespace views {
class Widget;
}

// The Views implementation of OverlayWindow.
class OverlayWindowViews : public OverlayWindow {
 public:
  explicit OverlayWindowViews(PictureInPictureWindowController* controller);
  ~OverlayWindowViews() override;

  // OverlayWindow:
  void Init(const gfx::Size& size) override;

 private:
  std::unique_ptr<views::Widget> widget_;

  DISALLOW_COPY_AND_ASSIGN(OverlayWindowViews);
};

#endif  // CHROME_BROWSER_UI_VIEWS_OVERLAY_OVERLAY_WINDOW_VIEWS_H_
