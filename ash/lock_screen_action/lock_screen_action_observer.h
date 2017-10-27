// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_LOCK_SCREEN_ACTION_LOCK_SCREEN_ACTION_OBSERVER_H_
#define ASH_LOCK_SCREEN_ACTION_LOCK_SCREEN_ACTION_OBSERVER_H_

#include "ash/ash_export.h"
#include "ash/public/interfaces/lock_screen_action.mojom.h"

namespace ash {

class ASH_EXPORT LockScreenActionObserver {
 public:
  virtual ~LockScreenActionObserver() {}

  // Called when action handler state changes.
  virtual void OnNoteStateChanged(mojom::LockScreenActionState state) = 0;
};

}  // namespace ash

#endif  // ASH_LOCK_SCREEN_ACTION_LOCK_SCREEN_ACTION_OBSERVER_H_
