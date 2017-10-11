// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/lock_screen_action/lock_screen_action_background_controller_impl_test_api.h"

#include "ash/lock_screen_action/lock_screen_action_background_controller_impl.h"

namespace ash {

LockScreenActionBackgroundControllerImplTestApi::
    LockScreenActionBackgroundControllerImplTestApi(
        LockScreenActionBackgroundControllerImpl* controller)
    : controller_(controller) {}

LockScreenActionBackgroundControllerImplTestApi::
    ~LockScreenActionBackgroundControllerImplTestApi() = default;

LockScreenActionBackgroundView*
LockScreenActionBackgroundControllerImplTestApi::GetContentsView() {
  return controller_->contents_view_;
}

views::Widget* LockScreenActionBackgroundControllerImplTestApi::GetWidget() {
  return controller_->background_widget_;
}

}  // namespace ash
