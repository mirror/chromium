// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_POWER_SCOPED_DISPLAY_FORCED_OFF_H_
#define ASH_SYSTEM_POWER_SCOPED_DISPLAY_FORCED_OFF_H_

#include "base/callback.h"
#include "base/macros.h"

namespace ash {

class ScopedDisplayForcedOff {
 public:
  using UnregisterCallback = base::OnceCallback<void(ScopedDisplayForcedOff*)>;
  explicit ScopedDisplayForcedOff(UnregisterCallback unregister_callback);
  ~ScopedDisplayForcedOff();

  void Reset();

  bool IsActive() const;

 private:
  // Callback that should be called in order for |this| to stop forcing the
  // display off.
  UnregisterCallback unregister_callback_;

  DISALLOW_COPY_AND_ASSIGN(ScopedDisplayForcedOff);
};

}  // namespace ash

#endif  // ASH_SYSTEM_POWER_SCOPED_DISPLAY_FORCED_OFF_H_
