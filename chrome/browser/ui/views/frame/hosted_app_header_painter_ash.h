// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_FRAME_HOSTED_APP_HEADER_PAINTER_ASH_H_
#define CHROME_BROWSER_UI_VIEWS_FRAME_HOSTED_APP_HEADER_PAINTER_ASH_H_

#include "ash/frame/default_header_painter.h"
#include "base/macros.h"

namespace gfx {
class Canvas;
}

namespace extensions {
class HostedAppBrowserController;
}

// Helper class for painting the hosted app window header.
class HostedAppHeaderPainterAsh : public ash::DefaultHeaderPainter {
 public:
  HostedAppHeaderPainterAsh(
      extensions::HostedAppBrowserController* app_controller);
  ~HostedAppHeaderPainterAsh() override;

 private:
  // ash::DefaultHeaderPainter:
  void PaintTitleBar(gfx::Canvas* canvas) override;

  extensions::HostedAppBrowserController* app_controller_;  // Weak.

  DISALLOW_COPY_AND_ASSIGN(HostedAppHeaderPainterAsh);
};

#endif  // CHROME_BROWSER_UI_VIEWS_FRAME_HOSTED_APP_HEADER_PAINTER_ASH_H_
