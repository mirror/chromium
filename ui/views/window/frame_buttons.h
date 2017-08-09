// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_WINDOW_FRAME_BUTTONS_H_
#define UI_VIEWS_WINDOW_FRAME_BUTTONS_H_

namespace views {

// Identifies what a button in a window frame is.
enum FrameButton {
  FRAME_BUTTON_MINIMIZE,
  // FRAME_BUTTON_MAXIMIZE could either be a maximize or a restore
  // button depending on the context.
  FRAME_BUTTON_MAXIMIZE,
  FRAME_BUTTON_CLOSE
};

enum class FrameButtonDisplayType {
  MINIMIZE = 0,
  MAXIMIZE,
  RESTORE,
  CLOSE,
  MAX
};

}  // namespace views

#endif  // UI_VIEWS_WINDOW_FRAME_BUTTONS_H_
