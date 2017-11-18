// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_ARC_TIMER_ARC_TIMER_H_
#define COMPONENTS_ARC_TIMER_ARC_TIMER_H_

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
  ~ArcTimer() override = default;

  // Override from timer.mojom |Timer::Start|.
  void Start(int64_t nanoseconds_from_now, StartCallback callback) override;

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

}  // namespace arc

#endif  // COMPONENTS_ARC_TIMER_ARC_TIMER_H_
