// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_OVERLAY_OVERLAY_WINDOW_H_
#define CHROME_BROWSER_UI_OVERLAY_OVERLAY_WINDOW_H_

#include "base/memory/ptr_util.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/views/widget/widget.h"

namespace gfx {
class Rect;
class Size;
}

namespace ui {
class Layer;
}

class PictureInPictureWindowController;

// This window will always float above other windows. The intention is to show
// content perpetually while the user is still interacting with the other
// browser windows.
class OverlayWindow : public views::Widget {
 public:
  // TODO: move to cc file.
  explicit OverlayWindow(PictureInPictureWindowController* controller)
    : controller_(controller) {}
  ~OverlayWindow() override = default;

  // Returns a created OverlayWindow. This is defined in the platform-specific
  // implementation for the class.
  static std::unique_ptr<OverlayWindow> Create(PictureInPictureWindowController* controller); 
  
  // TODO: probably wrap into ::Create() or something.
  virtual void Init(const gfx::Size& size) = 0;

 protected:
    // Weak pointer because |controller_| owns |this|.
    // TODO: we could have an "observer" interface if we think it's cleaner.
    PictureInPictureWindowController* controller_;  

 private:
  DISALLOW_COPY_AND_ASSIGN(OverlayWindow);
};

#endif  // CHROME_BROWSER_UI_OVERLAY_OVERLAY_WINDOW_H_
