// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_FRAME_HOSTED_APP_FRAME_HEADER_ASH_H_
#define CHROME_BROWSER_UI_VIEWS_FRAME_HOSTED_APP_FRAME_HEADER_ASH_H_

#include "ash/frame/default_frame_header.h"
#include "base/macros.h"

namespace gfx {
class Canvas;
}

namespace extensions {
class HostedAppBrowserController;
}

// Helper class for managing the hosted app window header.
class HostedAppFrameHeaderAsh : public ash::DefaultFrameHeader {
 public:
  HostedAppFrameHeaderAsh(
      extensions::HostedAppBrowserController* app_controller,
      views::Widget* frame,
      views::View* header_view,
      ash::FrameCaptionButtonContainerView* caption_button_container,
      ash::FrameCaptionButton* back_button);
  ~HostedAppFrameHeaderAsh() override;

 private:
  // ash::DefaultFrameHeader:
  void PaintTitleBar(gfx::Canvas* canvas) override;

  extensions::HostedAppBrowserController* app_controller_;  // Weak.

  DISALLOW_COPY_AND_ASSIGN(HostedAppFrameHeaderAsh);
};

#endif  // CHROME_BROWSER_UI_VIEWS_FRAME_HOSTED_APP_FRAME_HEADER_ASH_H_
