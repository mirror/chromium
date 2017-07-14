// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_WM_MAXIMIZE_MODE_MAXIMIE_MODE_OBSERVER_H_
#define ASH_WM_MAXIMIZE_MODE_MAXIMIE_MODE_OBSERVER_H_

#include "ash/ash_export.h"

namespace ash {

// Used to observe maximize mode changes.
class ASH_EXPORT MaximizeModeObserver {
 public:
  // Called when the always maximize mode has started. Windows might still
  // animate though.
  virtual void OnMaximizeModeStarted() {}

  // Called when the maximize mode is about to end.
  virtual void OnMaximizeModeEnding() {}

  // Called when the maximize mode has ended. Windows may still be
  // animating but have been restored.
  virtual void OnMaximizeModeEnded() {}

 protected:
  virtual ~MaximizeModeObserver() {}
};

}  // namespace ash

#endif  // ASH_WM_MAXIMIZE_MODE_MAXIMIE_MODE_OBSERVER_H_
