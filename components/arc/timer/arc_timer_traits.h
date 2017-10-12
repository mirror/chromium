// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_ARC_TIMER_ARC_TIMER_TRAITS_H_
#define COMPONENTS_ARC_TIMER_ARC_TIMER_TRAITS_H_

#include "chromeos/dbus/power_manager_client.h"
#include "components/arc/common/timer.mojom.h"

namespace {
int32_t kInvalidClockId = -1;
}

namespace mojo {

template <>
struct StructTraits<arc::mojom::ArcTimerArgsDataView,
                    chromeos::PowerManagerClient::ArcTimerArgs> {
  // Stub function as the reverse translation is not
  // required.
  static int32_t clock_id(
      const chromeos::PowerManagerClient::ArcTimerArgs& arc_timer_args) {
    return kInvalidClockId;
  }

  // Stub function as the reverse translation is not
  // required.
  static mojo::ScopedHandle expiry_indicator_fd(
      const chromeos::PowerManagerClient::ArcTimerArgs& arc_timer_args) {
    return mojo::ScopedHandle(Handle());
  }

  static bool Read(arc::mojom::ArcTimerArgsDataView input,
                   chromeos::PowerManagerClient::ArcTimerArgs* output);
};
}  // namespace mojo

#endif  // COMPONENTS_ARC_TIMER_ARC_TIMER_TRAITS_H_
