// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/libgtkui/nav_button_layout_manager.h"

#include "base/strings/string_tokenizer.h"

namespace libgtkui {

void NavButtonLayoutManager::ParseButtonLayout(
    const std::string& button_string,
    std::vector<views::FrameButton>* leading_buttons,
    std::vector<views::FrameButton>* trailing_buttons) {
  leading_buttons->clear();
  trailing_buttons->clear();
  bool left_side = true;
  base::StringTokenizer tokenizer(button_string, ":,");
  tokenizer.set_options(base::StringTokenizer::RETURN_DELIMS);
  while (tokenizer.GetNext()) {
    if (tokenizer.token_is_delim()) {
      if (*tokenizer.token_begin() == ':')
        left_side = false;
    } else {
      base::StringPiece token = tokenizer.token_piece();
      if (token == "minimize") {
        (left_side ? leading_buttons : trailing_buttons)
            ->push_back(views::FRAME_BUTTON_MINIMIZE);
      } else if (token == "maximize") {
        (left_side ? leading_buttons : trailing_buttons)
            ->push_back(views::FRAME_BUTTON_MAXIMIZE);
      } else if (token == "close") {
        (left_side ? leading_buttons : trailing_buttons)
            ->push_back(views::FRAME_BUTTON_CLOSE);
      }
    }
  }
}

}  // namespace libgtkui
