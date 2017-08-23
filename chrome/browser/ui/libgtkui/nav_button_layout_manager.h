// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_LIBGTKUI_NAV_BUTTON_LAYOUT_MANAGER_H_
#define CHROME_BROWSER_UI_LIBGTKUI_NAV_BUTTON_LAYOUT_MANAGER_H_

#include <string>
#include <vector>

#include "ui/views/window/frame_buttons.h"

namespace libgtkui {

class NavButtonLayoutManager {
 public:
  virtual ~NavButtonLayoutManager() {}

 protected:
  // Even though this class is not pure virtual, it should not be
  // instantiated directly.  Use NavButtonLayoutManagerGconf or
  // NavButtonLayoutManagerGtk3 instead.
  NavButtonLayoutManager() {}

  // Helper function for subclasses to parse |button_string|.
  void ParseButtonLayout(const std::string& button_string,
                         std::vector<views::FrameButton>* leading_buttons,
                         std::vector<views::FrameButton>* trailing_buttons);
};

}  // namespace libgtkui

#endif  // CHROME_BROWSER_UI_LIBGTKUI_NAV_BUTTON_LAYOUT_MANAGER_H_
