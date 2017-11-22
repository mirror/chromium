// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_ARC_TIMER_ARC_TIMER_H_
#define COMPONENTS_ARC_TIMER_ARC_TIMER_H_

#include <stdint.h>

#include <memory>
#include <set>

#include "base/files/file.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "components/arc/common/timer.mojom.h"
#include "components/arc/timer/arc_timer_request.h"
#include "components/timers/alarm_timer_chromeos.h"
#include "mojo/public/cpp/bindings/binding.h"

namespace arc {

// Implements the timer interface exported to the instance. An object of this
// class is accessed from the main thread as well as blocking threads.
class ArcTimer : public mojom::Timer {
 public:
  ArcTimer(base::ScopedFD expiration_fd,
           mojo::InterfaceRequest<mojom::Timer> request);
  ~ArcTimer() override = default;

  // mojom::Timer overrides.
  void Start(int64_t nanoseconds_from_now, StartCallback callback) override;

  // Encapsulates a ScopedFD in a ref counted thread safe class.
  class RefCountedScopedFd
      : public base::RefCountedThreadSafe<RefCountedScopedFd> {
   public:
    explicit RefCountedScopedFd(base::ScopedFD fd);

    // Returns the raw file descriptor being encapsulated.
    inline int get() const { return fd_.get(); }

   private:
    // The file descriptor being encapsulated.
    base::ScopedFD fd_;

    DISALLOW_COPY_AND_ASSIGN(RefCountedScopedFd);
  };

 private:
  // Handles connection errors with the instance.
  void ConnectionErrorHandler();

  mojo::Binding<mojom::Timer> binding_;

  // The file descriptor which will be written to when |timer| expires. This is
  // accessed from both main thread and blocking threads. A |scoped_refptr|
  // ensures that this can be accessed from the blocking thread if the owning
  // |ArcTimer| object goes away.
  scoped_refptr<RefCountedScopedFd> expiration_fd_;

  // The timer that will be scheduled. Only accessed from main thread.
  timers::SimpleAlarmTimer timer_;

  base::WeakPtrFactory<ArcTimer> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(ArcTimer);
};

}  // namespace arc

#endif  // COMPONENTS_ARC_TIMER_ARC_TIMER_H_
