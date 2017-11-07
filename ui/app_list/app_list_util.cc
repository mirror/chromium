// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/app_list/app_list_util.h"

namespace app_list {

bool CanProcessLeftRightKeyTraversal(const ui::KeyEvent& event) {
  if (event.handled() || event.type() != ui::ET_KEY_PRESSED)
    return false;

  if (event.key_code() != ui::VKEY_LEFT && event.key_code() != ui::VKEY_RIGHT)
    return false;

  if (event.IsShiftDown() || event.IsControlDown() || event.IsAltDown())
    return false;

  return true;
}

bool CanProcessUpDownKeyTraversal(const ui::KeyEvent& event) {
  if (event.handled() || event.type() != ui::ET_KEY_PRESSED)
    return false;

  if (event.key_code() != ui::VKEY_UP && event.key_code() != ui::VKEY_DOWN)
    return false;

  if (event.IsShiftDown() || event.IsControlDown() || event.IsAltDown())
    return false;

  return true;
}

}  // namespace app_list
