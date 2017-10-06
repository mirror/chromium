// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_FULLSCREEN_CONTROL_FULLSCREEN_CONTROL_CONTEXT_H_
#define CHROME_BROWSER_UI_VIEWS_FULLSCREEN_CONTROL_FULLSCREEN_CONTROL_CONTEXT_H_

#include "base/macros.h"

// Context in which the fullscreen control host is initiated.
class FullscreenControlContext {
 public:
  virtual ~FullscreenControlContext(){};

  // Returns the view used to parent the bubble Widget.
  virtual gfx::NativeView GetBubbleParentView() const = 0;

  // Return the current bounds (not restored bounds) of the parent window.
  virtual gfx::Rect GetClientAreaBoundsInScreen() const = 0;

  // Returns true if the window hosting the exclusive access bubble is
  // fullscreen.
  virtual bool IsFullscreen() const = 0;

  // Exits fullscreen and update exit bubble.
  virtual void ExitFullscreen() = 0;
};

#endif  // CHROME_BROWSER_UI_VIEWS_FULLSCREEN_CONTROL_FULLSCREEN_CONTROL_CONTEXT_H_
