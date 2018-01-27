// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/browser_window.h"

#include "ui/base/base_window.h"

// static
ui::WindowShowState BrowserWindow::GetShowState(const BrowserWindow* window) {
  if (window->IsFullscreen())
    return ui::SHOW_STATE_FULLSCREEN;
  if (window->IsMaximized())
    return ui::SHOW_STATE_MAXIMIZED;
  if (window->IsMinimized())
    return ui::SHOW_STATE_MINIMIZED;
  return ui::SHOW_STATE_NORMAL;
}
