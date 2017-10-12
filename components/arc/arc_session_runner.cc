// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/arc/arc_session_runner.h"

#include "base/logging.h"
#include "base/metrics/histogram_macros.h"
#include "base/task_runner.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "components/arc/arc_util.h"

namespace arc {

namespace {

// These enums are used to define the buckets for an enumerated UMA histogram
// and need to be synced with histograms.xml. This enum class should also be
// treated as append-only.
enum class ArcInstanceCrashEvent : int {
  // An ARC instance (of any kind) is started. We record this as a baseline.
  INSTANCE_STARTED = 0,
  // The instance crashed before establishing an IPC connection to Chrome.
  INSTANCE_CRASHED_EARLY = 1,
  // The instance crashed after establishing the connection.
  INSTANCE_CRASHED = 2,
  COUNT
};

constexpr base::TimeDelta kDefaultRestartDelay =
    base::TimeDelta::FromSeconds(5);

chromeos::SessionManagerClient* GetSessionManagerClient() {
  // If the DBusThreadManager or the SessionManagerClient aren't available,
  // there isn't much we can do. This should only happen when running tests.
  if (!chromeos::DBusThreadManager::IsInitialized() ||
      !chromeos::DBusThreadManager::Get() ||
      !chromeos::DBusThreadManager::Get()->GetSessionManagerClient())
    return nullptr;
  return chromeos::DBusThreadManager::Get()->GetSessionManagerClient();
}

void RecordInstanceCrashUma(ArcInstanceCrashEvent sample) {
  UMA_HISTOGRAM_ENUMERATION("Arc.ContainerCrash", sample,
                            ArcInstanceCrashEvent::COUNT);
}

void RecordInstanceRestartAfterCrashUma(size_t restart_after_crash_count) {
  if (!restart_after_crash_count)
    return;
  UMA_HISTOGRAM_COUNTS_1000("Arc.ContainerRestartAfterCrashCount",
                            restart_after_crash_count);
}

bool IsCrashUmaRecordingNeeded(size_t restart_after_crash_count,
                               ArcStopReason stop_reason,
                               bool was_running,
                               ArcInstanceCrashEvent* out_uma_to_record) {
  if (stop_reason != ArcStopReason::CRASH)
    return false;

  if (!was_running) {
    // Always record early crashes. Restarting is not available at this point.
    *out_uma_to_record = ArcInstanceCrashEvent::INSTANCE_CRASHED_EARLY;
    return true;
  }

  // Record UMA only when this is the first non-early crash.
  if (restart_after_crash_count != 0)
    return false;

  *out_uma_to_record = ArcInstanceCrashEvent::INSTANCE_CRASHED;
  return true;
}

// Returns true if restart is needed for given conditions.
bool IsRestartNeeded(bool run_requested,
                     ArcStopReason stop_reason,
                     bool was_running) {
  if (!run_requested) {
    // The request to run ARC is canceled by the caller. No need to restart.
    return false;
  }

  switch (stop_reason) {
    case ArcStopReason::SHUTDOWN:
      // This is a part of stop requested by ArcSessionRunner.
      // If ARC is re-requested to start, restart is necessary.
      // This case happens, e.g., RequestStart() -> RequestStop() ->
      // RequestStart(), case. If the second RequestStart() is called before
      // the instance previously running is stopped, then just |run_requested|
      // flag is set. On completion, restart is needed.
      return true;
    case ArcStopReason::GENERIC_BOOT_FAILURE:
    case ArcStopReason::LOW_DISK_SPACE:
      // These two are errors on starting. To prevent failure loop, do not
      // restart.
      return false;
    case ArcStopReason::CRASH:
      // ARC instance is crashed unexpectedly, so automatically restart.
      // However, to avoid crash loop, do not restart if it is not successfully
      // started yet. So, check |was_running|.
      return was_running;
  }

  NOTREACHED();
  return false;
}

}  // namespace

ArcSessionRunner::ArcSessionRunner(const ArcSessionFactory& factory)
    : restart_delay_(kDefaultRestartDelay),
      restart_after_crash_count_(0),
      factory_(factory),
      weak_ptr_factory_(this) {
  chromeos::SessionManagerClient* client = GetSessionManagerClient();
  if (client)
    client->AddObserver(this);
}

ArcSessionRunner::~ArcSessionRunner() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  if (arc_session_)
    arc_session_->RemoveObserver(this);
  chromeos::SessionManagerClient* client = GetSessionManagerClient();
  if (client)
    client->RemoveObserver(this);
}

void ArcSessionRunner::AddObserver(Observer* observer) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  observer_list_.AddObserver(observer);
}

void ArcSessionRunner::RemoveObserver(Observer* observer) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  observer_list_.RemoveObserver(observer);
}

void ArcSessionRunner::RequestStart() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  // Consecutive RequestStart() call. Do nothing.
  if (run_requested_)
    return;

  VLOG(1) << "Session start requested";
  run_requested_ = true;
  // Here |run_requested_| transitions from false to true. So, |restart_timer_|
  // must be stopped (either not even started, or has been cancelled in
  // previous RequestStop() call).
  DCHECK(!restart_timer_.IsRunning());

  if (arc_session_ && arc_session_->IsStopRequested()) {
    // This is the case where RequestStop() was called, but before
    // |arc_session_| had finshed stopping, RequestStart() is called.
    // Do nothing in the that case, since when |arc_session_| does actually
    // stop, OnSessionStopped() will be called, where it should automatically
    // restart.
    return;
  }

  StartArcSession();
}

void ArcSessionRunner::RequestStop() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  // Unlike RequestStart(), this does not check |run_requested_| here.
  // Even if |run_requested_| is false, an instance for login screen may be
  // running, which should stop here.
  VLOG(1) << "Session stop requested";
  run_requested_ = false;

  if (arc_session_) {
    // If |arc_session_| is running, stop it.
    // Note that |arc_session_| may be already in the process of stopping or
    // be stopped.
    // E.g. RequestStart() -> RequestStop() -> RequestStart() -> RequestStop()
    // case. If the second RequestStop() is called before the first
    // RequestStop() is not yet completed for the instance, Stop() of the
    // instance is called again, but it is no-op as expected.
    arc_session_->Stop();
  }

  // In case restarting is in progress, cancel it.
  restart_timer_.Stop();
}

void ArcSessionRunner::OnShutdown() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  VLOG(1) << "OnShutdown";
  run_requested_ = false;
  restart_timer_.Stop();
  if (arc_session_)
    arc_session_->OnShutdown();
  // ArcSession::OnShutdown() invokes OnSessionStopped() synchronously.
  // In the observer method, |arc_session_| should be destroyed.
  DCHECK(!arc_session_);
}

void ArcSessionRunner::SetRestartDelayForTesting(
    const base::TimeDelta& restart_delay) {
  DCHECK(!arc_session_);
  DCHECK(!restart_timer_.IsRunning());
  restart_delay_ = restart_delay;
}

void ArcSessionRunner::StartArcSession() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(!restart_timer_.IsRunning());

  VLOG(1) << "Starting ARC instance";
  if (!arc_session_) {
    arc_session_ = factory_.Run();
    arc_session_->AddObserver(this);
  } else {
    DCHECK(arc_session_->IsForLoginScreen());
  }
  arc_session_->Start();
}

void ArcSessionRunner::RestartArcSession() {
  VLOG(0) << "Restarting ARC instance";
  // The order is important here. Call StartArcSession(), then notify observers.
  StartArcSession();
  for (auto& observer : observer_list_)
    observer.OnSessionRestarting();
}

void ArcSessionRunner::OnSessionStarting() {
  RecordInstanceCrashUma(ArcInstanceCrashEvent::INSTANCE_STARTED);
}

void ArcSessionRunner::OnSessionStopped(ArcStopReason stop_reason,
                                        bool was_running) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(arc_session_);
  DCHECK(!restart_timer_.IsRunning());

  VLOG(0) << "ARC stopped: " << stop_reason;

  ArcInstanceCrashEvent uma_to_record;
  if (IsCrashUmaRecordingNeeded(restart_after_crash_count_, stop_reason,
                                was_running, &uma_to_record)) {
    RecordInstanceCrashUma(uma_to_record);
  }

  // The observers should be agnostic to the existence of the limited-purpose
  // instance.
  const bool notify_observers = !arc_session_->IsForLoginScreen();

  arc_session_->RemoveObserver(this);
  arc_session_.reset();

  const bool restarting =
      IsRestartNeeded(run_requested_, stop_reason, was_running);

  if (restarting && stop_reason == ArcStopReason::CRASH)
    ++restart_after_crash_count_;
  else
    restart_after_crash_count_ = 0;  // session ended.

  if (restarting) {
    RecordInstanceRestartAfterCrashUma(restart_after_crash_count_);

    // There was a previous invocation and it crashed for some reason. Try
    // starting ARC instance later again.
    // Note that even |restart_delay_| is 0 (for testing), it needs to
    // PostTask, because observer callback may call RequestStart()/Stop().
    VLOG(0) << "ARC restarting";
    restart_timer_.Start(FROM_HERE, restart_delay_,
                         base::Bind(&ArcSessionRunner::RestartArcSession,
                                    weak_ptr_factory_.GetWeakPtr()));
  }

  if (notify_observers) {
    for (auto& observer : observer_list_)
      observer.OnSessionStopped(stop_reason, restarting);
  }
}

void ArcSessionRunner::EmitLoginPromptVisibleCalled() {
  if (ShouldArcOnlyStartAfterLogin()) {
    // Skip starting ARC for now. We'll have another chance to start the full
    // instance after the user logs in.
    return;
  }
  // Since 'login-prompt-visible' Upstart signal starts all Upstart jobs the
  // container may depend on such as cras, EmitLoginPromptVisibleCalled() is the
  // safe place to start the container for login screen.
  DCHECK(!arc_session_);
  arc_session_ = factory_.Run();
  arc_session_->AddObserver(this);
  arc_session_->StartForLoginScreen();
}

}  // namespace arc
