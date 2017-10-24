// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_ARC_TIMER_ARC_TIMER_IMPL_H_
#define COMPONENTS_ARC_TIMER_ARC_TIMER_IMPL_H_

#include <stdint.h>

#include <map>
#include <memory>
#include <set>

#include <base/files/file.h>
#include <components/timers/alarm_timer_chromeos.h>

#include "components/arc/common/timer.mojom.h"
#include "components/arc/timer/arc_timer_args.h"

namespace arc_timer {

struct ArcTimer {
  ~ArcTimer();

  // The timer that will be scheduled.
  std::unique_ptr<timers::SimpleAlarmTimer> timer;

  // The file descriptor which will be written to when |timer| expires.
  base::ScopedFD expiration_fd;
};

class ArcTimerImpl {
 public:
  ArcTimerImpl();

  arc::mojom::ArcTimerResult CreateArcTimers(
      std::vector<arc::ArcTimerArgs> args);

  arc::mojom::ArcTimerResult SetArcTimer(int32_t clock_id,
                                         base::TimeDelta time_delta);

  arc::mojom::ArcTimerResult DeleteArcTimers();

 private:
  // Callback for timer of type |clock_id| set on behalf of the instance.
  void ArcTimerCallback(int32_t clock_id);

  // Finds |ArcTimer| entry in |arc_timer_store_| corresponding to |clock_id|.
  // Returns null if an entry is not possible. Returns non-null pointer
  // otherwise.
  ArcTimer* FindArcTimer(int32_t clock_id);

  // Returns true if the instance has set up timers; false otherwise.
  bool ArcTimersInitialized() const;

  // Map that stores |AlarmTimer|s corresponding to different clocks used by
  // the instance. Each clock type has only one timer associated with it.
  std::map<int32_t, std::unique_ptr<ArcTimer>> arc_timer_store_;

  // The type of clocks whose timers can be created for the instance. Currently,
  // only CLOCK_REALTIME_ALARM and CLOCK_BOOTTIME_ALARM is supported.
  std::set<int32_t> arc_supported_timer_clocks_;
};

}  // namespace arc_timer

#endif  // COMPONENTS_ARC_TIMER_ARC_TIMER_IMPL_H_
