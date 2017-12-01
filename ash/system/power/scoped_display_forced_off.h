// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_POWER_SCOPED_DISPLAY_FORCED_OFF_H_
#define ASH_SYSTEM_POWER_SCOPED_DISPLAY_FORCED_OFF_H_

#include "ash/ash_export.h"
#include "base/callback.h"
#include "base/macros.h"

namespace ash {

// Utility class for keeping request to force display off active.
// An instance of this class is returned by
// DisplayForcedOffSetter::ForceDisplayOff - display will be kept in forced off
// state until the returned object goes out of scope or is |Reset|.
class ASH_EXPORT ScopedDisplayForcedOff {
 public:
  using UnregisterCallback = base::OnceCallback<void(ScopedDisplayForcedOff*)>;
  explicit ScopedDisplayForcedOff(UnregisterCallback unregister_callback);
  ~ScopedDisplayForcedOff();

  // Resets this object.
  void Reset();

  // Whether this object is still active, and keeping display forced off.
  bool IsActive() const;

 private:
  // Callback that should be called in order for |this| to unregister diplay
  // forced off request.
  UnregisterCallback unregister_callback_;

  DISALLOW_COPY_AND_ASSIGN(ScopedDisplayForcedOff);
};

}  // namespace ash

#endif  // ASH_SYSTEM_POWER_SCOPED_DISPLAY_FORCED_OFF_H_
