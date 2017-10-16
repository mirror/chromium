// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_DISPLAY_MOVE_WINDOW_CONTROLLER_H_
#define ASH_DISPLAY_MOVE_WINDOW_CONTROLLER_H_

#include "ash/ash_export.h"
#include "base/macros.h"

namespace ash {

class ASH_EXPORT MoveWindowController {
 public:
  MoveWindowController();
  ~MoveWindowController();

  void HandleMoveWindowToLeftDisplay();

 private:
  int64_t GetNextLet(int64_t display_id);

  DISALLOW_COPY_AND_ASSIGN(MoveWindowController);
};

}  // namespace ash

#endif  // ASH_DISPLAY_MOVE_WINDOW_CONTROLLER_H_
