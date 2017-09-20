// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_STATUS_AREA_AND_SHELF_FOCUS_OBSERVER_H_
#define ASH_SYSTEM_STATUS_AREA_AND_SHELF_FOCUS_OBSERVER_H_

#include "ash/ash_export.h"
#include "base/macros.h"

namespace ash {

// A class that observes status area and shelf related focus events.
class ASH_EXPORT StatusAreaAndShelfFocusObserver {
 public:
  // Called when focus is about to leave status area and goes forward.
  virtual void OnFocusAboutToLeaveStatusArea() = 0;

  // Called when focus is about to leave shelf and goes backward.
  virtual void OnFocusAboutToLeaveShelf() = 0;

 protected:
  virtual ~StatusAreaAndShelfFocusObserver() {}
};

}  // namespace ash

#endif  // ASH_SYSTEM_STATUS_AREA_AND_SHELF_FOCUS_OBSERVER_H_
