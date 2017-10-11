// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/lock_screen_action/lock_screen_action_background_view_test_api.h"

#include "ash/lock_screen_action/lock_screen_action_background_view.h"

namespace ash {

LockScreenActionBackgroundViewTestApi::LockScreenActionBackgroundViewTestApi(
    LockScreenActionBackgroundView* action_background_view)
    : action_background_view_(action_background_view) {}

LockScreenActionBackgroundViewTestApi::
    ~LockScreenActionBackgroundViewTestApi() = default;

views::View* LockScreenActionBackgroundViewTestApi::GetBackground() {
  return action_background_view_->GetBackgroundView();
}

}  // namespace ash
