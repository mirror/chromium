// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/power/scoped_display_forced_off.h"

#include <utility>

namespace ash {

ScopedDisplayForcedOff::ScopedDisplayForcedOff(
    UnregisterCallback unregister_callback)
    : unregister_callback_(std::move(unregister_callback)) {}

ScopedDisplayForcedOff::~ScopedDisplayForcedOff() {
  Reset();
}

void ScopedDisplayForcedOff::Reset() {
  if (unregister_callback_)
    std::move(unregister_callback_).Run(this);
}

bool ScopedDisplayForcedOff::IsActive() const {
  return unregister_callback_ && !unregister_callback_.is_null();
}

}  // namespace ash
