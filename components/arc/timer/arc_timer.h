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
#include "base/observer_list.h"
#include "base/task_scheduler/post_task.h"
#include "base/time/time.h"
#include "components/arc/common/timer.mojom.h"
#include "components/arc/timer/create_timer_request.h"
#include "components/timers/alarm_timer_chromeos.h"
#include "mojo/public/cpp/bindings/binding.h"

namespace arc {

// Implements the timer interface exported to the instance. All accesses to an
// instance of this class must happen on a single sequence that supports
// blocking operations and the |base::FileDescriptorWatcher| API.
//
// In the current implementation, |ArcTimer| objects are bound to a mojo proxy
// on a separate task runner in |arc_timer_bridge.cc|. Mojo guarantees to call
// all callbacks on the task runner that the mojo::Binding was created on.
// Consequently, even the timer expiration i.e. |OnExpiration| happens on the
// task runner and is allowed to do I/O.
class ArcTimer : public mojom::Timer {
 public:
  ArcTimer(base::ScopedFD expiration_fd,
           mojo::InterfaceRequest<mojom::Timer> request);
  ~ArcTimer() override;

  // mojom::Timer overrides. Runs on separate task runner since this class is
  // bound to a mojo proxy on a separate task runner.
  void Start(base::TimeTicks absolute_expiration_time,
             StartCallback callback) override;

  class Observer {
   public:
    // Called when there is a mojo connection error in |timer|. Runs on the the
    // same thread on which |timer| was created.
    virtual void OnConnectionError(ArcTimer* timer);
  };

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

 private:
  // Handles connection errors with the instance.
  void HandleConnectionError();

  // Callback fired when the timer in |ArcTimer| expires.
  void OnExpiration();

  mojo::Binding<mojom::Timer> binding_;

  // The timer that will be scheduled. Only accessed from the sequence on which
  // this object is created.
  timers::SimpleAlarmTimer timer_;

  // The file descriptor which will be written to when |timer| expires. Only
  // accessed from the sequence on which this object is created.
  base::ScopedFD expiration_fd_;

  base::ObserverList<Observer, true> observers_;

  DISALLOW_COPY_AND_ASSIGN(ArcTimer);
};

}  // namespace arc

#endif  // COMPONENTS_ARC_TIMER_ARC_TIMER_H_
