// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_WM_TABLET_MODE_SCOPED_IGNORE_PROCESS_EVENTS_CHECKS_H_
#define ASH_WM_TABLET_MODE_SCOPED_IGNORE_PROCESS_EVENTS_CHECKS_H_

#include "base/macros.h"

namespace ash {

// ScopedIgnoreProcessEventsChecks allows for ignoring checks in the event
// client for a short region of code within a scope.
class ScopedIgnoreProcessEventsChecks {
 public:
  ScopedIgnoreProcessEventsChecks();
  ~ScopedIgnoreProcessEventsChecks();

 private:
  DISALLOW_COPY_AND_ASSIGN(ScopedIgnoreProcessEventsChecks);
};

}  // namespace ash

#endif  // ASH_WM_TABLET_MODE_SCOPED_IGNORE_PROCESS_EVENTS_CHECKS_H_
