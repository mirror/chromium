// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/workspace/backdrop_delegate.h"

#include "ash/accessibility/accessibility_controller.h"
#include "ash/public/cpp/app_types.h"
#include "ash/shell.h"
#include "ash/wm/window_util.h"
#include "ui/aura/client/aura_constants.h"

namespace ash {

bool BackdropDelegate::HasBackdrop(aura::Window* window) {
  if (window->GetAllPropertyKeys().count(aura::client::kHasBackdrop) &&
      window->GetProperty(aura::client::kHasBackdrop)) {
    return true;
  }

  // If |window| is the current active window and is an ARC app window, |window|
  // should have a backdrop when spoken feedback is enabled.
  if (window->GetProperty(aura::client::kAppType) ==
          static_cast<int>(AppType::ARC_APP) &&
      wm::IsActiveWindow(window) &&
      Shell::Get()->accessibility_controller()->IsSpokenFeedbackEnabled()) {
    return true;
  }

  return false;
}

}  // namespace ash
