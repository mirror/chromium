// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SHUTDOWN_CONTROLLER_OBSERVER_H_
#define ASH_SHUTDOWN_CONTROLLER_OBSERVER_H_

namespace ash {

class ShutdownControllerObserver {
 public:
  virtual ~ShutdownControllerObserver() {}

  // Called when shutdown policy changes.
  virtual void OnShutdownPolicyChanged(bool reboot_on_shutdown) = 0;
};

}  // namespace ash

#endif  // ASH_SHUTDOWN_CONTROLLER_OBSERVER_H_