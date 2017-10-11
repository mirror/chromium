// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_LOCK_SCREEN_ACTION_LOCK_SCREEN_ACTION_BACKGROUND_VIEW_TEST_API_H_
#define ASH_LOCK_SCREEN_ACTION_LOCK_SCREEN_ACTION_BACKGROUND_VIEW_TEST_API_H_

#include "ash/ash_export.h"
#include "base/macros.h"

namespace views {
class View;
}

namespace ash {

class LockScreenActionBackgroundView;

// Class that provides access to LockScreenActionBackgroundView implementation
// details in tests.
class ASH_EXPORT LockScreenActionBackgroundViewTestApi {
 public:
  explicit LockScreenActionBackgroundViewTestApi(
      LockScreenActionBackgroundView* action_background_view);
  ~LockScreenActionBackgroundViewTestApi();

  // Gets the view that paints the background contents.
  views::View* GetBackground();

 private:
  LockScreenActionBackgroundView* action_background_view_;

  DISALLOW_COPY_AND_ASSIGN(LockScreenActionBackgroundViewTestApi);
};

}  // namespace ash
#endif  // ASH_LOCK_SCREEN_ACTION_LOCK_SCREEN_ACTION_BACKGROUND_VIEW_TEST_API_H_
