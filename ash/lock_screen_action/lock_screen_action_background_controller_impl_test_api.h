// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_LOCK_SCREEN_ACTION_LOCK_SCREEN_ACTION_BACKGROUND_CONTROLLER_IMPL_TEST_API_H_
#define ASH_LOCK_SCREEN_ACTION_LOCK_SCREEN_ACTION_BACKGROUND_CONTROLLER_IMPL_TEST_API_H_

#include "ash/ash_export.h"
#include "base/macros.h"

namespace views {
class Widget;
}

namespace ash {

class LockScreenActionBackgroundControllerImpl;
class LockScreenActionBackgroundView;

// Class that provides access to LockScreenActionBackgroundControllerImpl
// implementation details in tests.
class ASH_EXPORT LockScreenActionBackgroundControllerImplTestApi {
 public:
  explicit LockScreenActionBackgroundControllerImplTestApi(
      LockScreenActionBackgroundControllerImpl* controller);
  ~LockScreenActionBackgroundControllerImplTestApi();

  // Retrieves the background widget, if it was created.
  views::Widget* GetWidget();

  // Retrieves the background widget's contents view, if the widget was
  // created.
  LockScreenActionBackgroundView* GetContentsView();

 private:
  LockScreenActionBackgroundControllerImpl* controller_;

  DISALLOW_COPY_AND_ASSIGN(LockScreenActionBackgroundControllerImplTestApi);
};

}  // namespace ash

#endif  // ASH_LOCK_SCREEN_ACTION_LOCK_SCREEN_ACTION_BACKGROUND_CONTROLLER_IMPL_TEST_API_H_
