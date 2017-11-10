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

class ArcTimer : public mojom::Timer {
 public:
  explicit ArcTimer(base::ScopedFD expiration_fd,
                    std::unique_ptr<timers::SimpleAlarmTimer> timer,
                    mojo::InterfaceRequest<mojom::Timer> request)
      : binding_(this, std::move(request)),
        expiration_fd_(std::move(expiration_fd)),
        timer_(std::move(timer)),
        weak_factory_(this) {}
  ~ArcTimer();

  void Start(int64_t seconds_from_now,
             int64_t nanoseconds_from_now,
             StartCallback callback) override;

 private:
  void ExpirationCallback();

  void IndicateExpiration();

  void LogExpirationIndicationStatus();

  mojo::Binding<mojom::Timer> binding_;

  // The file descriptor which will be written to when |timer| expires.
  base::ScopedFD expiration_fd_;

  // The timer that will be scheduled.
  std::unique_ptr<timers::SimpleAlarmTimer> timer_;

  base::WeakPtrFactory<ArcTimer> weak_factory_;
};

class ArcTimerImpl {
 public:
  ArcTimerImpl();

  std::vector<mojom::TimerPtr> CreateArcTimers(
      std::vector<arc::ArcTimerRequest> arc_timer_requests);

  void DeleteArcTimers();

 private:
  // The type of clocks whose timers can be created for the instance. Currently,
  // only CLOCK_REALTIME_ALARM and CLOCK_BOOTTIME_ALARM is supported.
  const std::set<int32_t> arc_supported_timer_clocks_;

  // Store of |ArcTimer| objects.
  std::set<std::unique_ptr<ArcTimer>> arc_timers_;
};

}  // namespace arc

#endif  // COMPONENTS_ARC_TIMER_ARC_TIMER_IMPL_H_
