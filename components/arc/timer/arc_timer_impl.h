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
#include "components/arc/timer/arc_timer_request.h"
#include "mojo/public/cpp/bindings/binding.h"

namespace arc {

// Implements the timer interface exported to the instance.
class ArcTimer : public mojom::Timer {
 public:
  explicit ArcTimer(base::ScopedFD expiration_fd,
                    mojo::InterfaceRequest<mojom::Timer> request);
  ~ArcTimer() override;

  // Start the timer to run at |seconds_from_now| + |nanoseconds_from_now|
  // seconds from now. If the timer is already running, it will be replaced.
  // Notification will be performed as an 8-byte write to the associated
  // expiration fd.
  void Start(int64_t seconds_from_now,
             int64_t nanoseconds_from_now,
             StartCallback callback) override;

 private:
  // Callback fired when this timer expires.
  void OnExpiration();

  // Indicates this timers expiration to the instance by writing to
  // |expiration_fd_|.
  void WriteExpiration();

  mojo::Binding<mojom::Timer> binding_;

  // The file descriptor which will be written to when |timer| expires.
  base::ScopedFD expiration_fd_;

  // The timer that will be scheduled.
  timers::SimpleAlarmTimer timer_;

  base::WeakPtrFactory<ArcTimer> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(ArcTimer);
};

class ArcTimerImpl {
 public:
  ArcTimerImpl();

  // Creates timers with the given arguments. Returns a vector of size zero on
  // failure. On success, returns a non-empty vector of |Timer| objects.
  std::vector<mojom::TimerPtr> CreateArcTimers(
      std::vector<arc::ArcTimerRequest> arc_timer_requests);

  // Deletes all timers created i.e. any pending timers are cancelled.
  void DeleteArcTimers();

 private:
  // The type of clocks whose timers can be created for the instance. Currently,
  // only CLOCK_REALTIME_ALARM and CLOCK_BOOTTIME_ALARM is supported.
  const std::set<int32_t> arc_supported_timer_clocks_;

  // Store of |ArcTimer| objects.
  std::set<std::unique_ptr<ArcTimer>> arc_timers_;

  DISALLOW_COPY_AND_ASSIGN(ArcTimerImpl);
};

}  // namespace arc

#endif  // COMPONENTS_ARC_TIMER_ARC_TIMER_IMPL_H_
