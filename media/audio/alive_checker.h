// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_AUDIO_ALIVE_CHECKER_H_
#define MEDIA_AUDIO_ALIVE_CHECKER_H_

#include <memory>

#include "base/callback_forward.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/sequenced_task_runner.h"
#include "base/timer/timer.h"
#include "media/audio/power_observer_helper.h"

namespace media {

// A class that checks if a client that is expected to have regular activity
// is alive. For example, audio streams expect regular callbacks from the
// operating system. The client informs regularly that it's alive by calling
// NotifyAlive(). At a certain interval the AliveChecker checks that it has been
// notified within a timeout period. If not, it runs a callback to inform about
// detecting dead. The callback is run once and further checking is stopped at
// detection.
//
// The AliveChecker can pause checking when suspending. Not doing so can for
// shorter timeouts cause false positives. The pausing relies on notifications
// from base::PowerObserver.
//
// It takes a task runner on which the object lives and the callback is run,
// with the expception of NotifyAlive() which can be called on any task runner.
//
// It supports stopping the checking after at first NotifyAlive(). This
// can be useful for example if the platform doesn't support suspend/resume
// notifications, as Linux.
class AliveChecker {
 public:
  AliveChecker(scoped_refptr<base::SequencedTaskRunner> task_runner,
               base::RepeatingClosure dead_callback,
               base::TimeDelta check_interval,
               base::TimeDelta timeout,
               bool stop_at_first_alive_notification,
               bool pause_check_during_suspend);

  ~AliveChecker();

  // Start and stop checking if alive.
  void Start();
  void Stop();

  // Returns whether dead was detected. Reset when Start() is called.
  bool DetectedDead();

  // Called regularly by the client to inform that it's alive. Can be called on
  // any thread.
  void NotifyAlive();

 private:
  // Checks if we have gotten an alive notification within a certain time
  // period. If not, run |dead_callback_|.
  void CheckIfAlive();

  // Sets |last_alive_notification_time_| to the current time.
  void SetLastAliveNotificationTimeToNowOnTaskRunner();

  // Resets last alive notification time.
  void OnResume();

  // Timer to run the check regularly.
  std::unique_ptr<base::RepeatingTimer> check_alive_timer_;

  // Stores the time for the alive notification.
  // TODO(grunell): Change from TimeTicks to Atomic32 and remove the task
  // posting in NotifyAlive(). The Atomic32 variable would have to
  // represent some time in seconds or tenths of seconds to be able to span over
  // enough time. Atomic64 cannot be used since it's not supported on 32-bit
  // platforms.
  base::TimeTicks last_alive_notification_time_;

  // The interval at which we check if alive.
  const base::TimeDelta check_interval_;

  // The timeout time before we notify about being dead.
  const base::TimeDelta timeout_;

  // Flags that dead was detected.
  bool detected_dead_ = false;

  // The task runner on which this object lives.
  scoped_refptr<base::SequencedTaskRunner> task_runner_;

  // Dead notification callback.
  base::RepeatingClosure dead_callback_;

  // Flags that we should stop checking after first alive notification.
  const bool stop_at_first_alive_notification_;

  // Used for getting suspend/resume notifications.
  std::unique_ptr<PowerObserverHelper> power_observer_;

  base::WeakPtrFactory<AliveChecker> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(AliveChecker);
};

}  // namespace media

#endif  // MEDIA_AUDIO_ALIVE_CHECKER_H_
