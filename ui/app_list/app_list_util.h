// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_APP_LIST_APP_LIST_UTIL_H_
#define UI_APP_LIST_APP_LIST_UTIL_H_

#include "ui/app_list/app_list_export.h"
#include "ui/events/event.h"

namespace app_list {

// Returns true if the key event can be handled to do left or right focus
// traversal.
APP_LIST_EXPORT bool CanProcessLeftRightKeyTraversal(const ui::KeyEvent& event);

// Returns true if the key event can be handled to do up or down focus
// traversal.
APP_LIST_EXPORT bool CanProcessUpDownKeyTraversal(const ui::KeyEvent& event);

}  // namespace app_list

#endif  // UI_APP_LIST_APP_LIST_UTIL_H_
