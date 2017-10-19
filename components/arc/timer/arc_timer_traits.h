// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_ARC_TIMER_ARC_TIMER_TRAITS_H_
#define COMPONENTS_ARC_TIMER_ARC_TIMER_TRAITS_H_

#include "chromeos/dbus/power_manager_client.h"
#include "components/arc/common/timer.mojom.h"

namespace mojo {

template <>
struct StructTraits<arc::mojom::ArcTimerArgsDataView,
                    chromeos::PowerManagerClient::ArcTimerArgs> {
  static int32_t clock_id(
      const chromeos::PowerManagerClient::ArcTimerArgs& arc_timer_args);

  static mojo::ScopedHandle expiration_fd(
      const chromeos::PowerManagerClient::ArcTimerArgs& arc_timer_args);

  static bool Read(arc::mojom::ArcTimerArgsDataView input,
                   chromeos::PowerManagerClient::ArcTimerArgs* output);
};

}  // namespace mojo

#endif  // COMPONENTS_ARC_TIMER_ARC_TIMER_TRAITS_H_
