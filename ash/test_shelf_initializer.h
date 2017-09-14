// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_TEST_SHELF_INITIALIZER_H_
#define ASH_TEST_SHELF_INITIALIZER_H_

#include "ash/shell_observer.h"
#include "base/macros.h"

namespace ash {

// A ShellObserver that sets the shelf alignment and auto hide behavior when the
// shelf is created, to simulate ChromeLauncherController's behavior.
class ShelfInitializer : public ShellObserver {
 public:
  ShelfInitializer();
  ~ShelfInitializer() override;

  // ShellObserver:
  void OnShelfCreatedForRootWindow(aura::Window* root_window) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(ShelfInitializer);
};

}  // namespace ash

#endif  // ASH_TEST_SHELF_INITIALIZER_H_
