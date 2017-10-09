// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/loader/IdlenessDetector.h"

#include "core/dom/Document.h"
#include "core/dom/TaskRunnerHelper.h"
#include "core/frame/LocalFrame.h"
#include "core/probe/CoreProbes.h"
#include "platform/instrumentation/resource_coordinator/FrameResourceCoordinator.h"
#include "platform/loader/fetch/ResourceFetcher.h"
#include "public/platform/Platform.h"

namespace blink {

void IdlenessDetector::Shutdown() {
  Stop();
  local_frame_ = nullptr;
}

void IdlenessDetector::DomContentLoadedEventFired() {
  if (!local_frame_)
    return;

  if (!task_observer_added_) {
    Platform::Current()->CurrentThread()->AddTaskTimeObserver(this);
    task_observer_added_ = true;
  }

  network_2_quiet_ = 0;
  network_0_quiet_ = 0;

  if (auto* frame_resource_coordinator =
          local_frame_->GetFrameResourceCoordinator()) {
    frame_resource_coordinator->SetProperty(
        resource_coordinator::mojom::PropertyType::kNetworkAlmostIdle, false);
  }

  // Start to observe long task idleness, set to current timestamp.
  long_task_idle_ = MonotonicallyIncreasingTime();
  if (auto* frame_resource_coordinator =
          local_frame_->GetFrameResourceCoordinator()) {
    frame_resource_coordinator->SetProperty(
        resource_coordinator::mojom::PropertyType::kLongTaskIdle, false);
  }

  OnDidLoadResource();
}

void IdlenessDetector::OnWillSendRequest() {
  if (!local_frame_)
    return;

  int request_count =
      local_frame_->GetDocument()->Fetcher()->ActiveRequestCount();
  // If we are above the allowed number of active requests, reset timers.
  if (network_2_quiet_ >= 0 && request_count > 2)
    network_2_quiet_ = 0;
  if (network_0_quiet_ >= 0 && request_count > 0)
    network_0_quiet_ = 0;
}

// This function is called when the number of active connections is decreased.
// Note that the number of active connections doesn't decrease monotonically.
void IdlenessDetector::OnDidLoadResource() {
  if (!local_frame_)
    return;

  // Document finishes parsing after DomContentLoadedEventEnd is fired,
  // check the status in order to avoid false signals.
  if (!local_frame_->GetDocument()->HasFinishedParsing())
    return;

  // If we already reported quiet time, bail out.
  if (network_0_quiet_ < 0 && network_2_quiet_ < 0)
    return;

  int request_count =
      local_frame_->GetDocument()->Fetcher()->ActiveRequestCount();
  // If we did not achieve either 0 or 2 active connections, bail out.
  if (request_count > 2)
    return;

  double timestamp = MonotonicallyIncreasingTime();
  // Arriving at =2 updates the quiet_2 base timestamp.
  // Arriving at <2 sets the quiet_2 base timestamp only if
  // it was not already set.
  if (request_count == 2 && network_2_quiet_ >= 0) {
    network_2_quiet_ = timestamp;
    network_2_quiet_start_time_ = timestamp;
  } else if (request_count < 2 && network_2_quiet_ == 0) {
    network_2_quiet_ = timestamp;
    network_2_quiet_start_time_ = timestamp;
  }

  if (request_count == 0 && network_0_quiet_ >= 0) {
    network_0_quiet_ = timestamp;
    network_0_quiet_start_time_ = timestamp;
  }

  if (!network_quiet_timer_.IsActive()) {
    network_quiet_timer_.StartOneShot(kNetworkQuietWatchdogSeconds,
                                      BLINK_FROM_HERE);
  }
}

void IdlenessDetector::WillProcessTask(double start_time) {
  // If we have idle time and we are kIdlenessWindowSeconds seconds past it,
  // emit idle signals.
  if (network_2_quiet_ > 0 &&
      start_time - network_2_quiet_ > kIdlenessWindowSeconds) {
    probe::lifecycleEvent(local_frame_->GetDocument(), "networkAlmostIdle",
                          network_2_quiet_start_time_);
    if (auto* frame_resource_coordinator =
            local_frame_->GetFrameResourceCoordinator()) {
      frame_resource_coordinator->SetProperty(
          resource_coordinator::mojom::PropertyType::kNetworkAlmostIdle, true);
    }
    local_frame_->GetDocument()->Fetcher()->OnNetworkQuiet();
    network_2_quiet_ = -1;
  }

  if (network_0_quiet_ > 0 &&
      start_time - network_0_quiet_ > kIdlenessWindowSeconds) {
    probe::lifecycleEvent(local_frame_->GetDocument(), "networkIdle",
                          network_0_quiet_start_time_);
    network_0_quiet_ = -1;
  }

  // If long task idleness detection has already been started and there's no
  // long task during kIdlenessWindowSeconds before this task is about to be
  // processed, emit long task idleness signal.
  if (long_task_idle_ > 0 &&
      start_time - long_task_idle_ >= kIdlenessWindowSeconds) {
    EmitLongTaskIdlenessSignal();
  }

  if (network_0_quiet_ < 0 && network_2_quiet_ < 0 && long_task_idle_ < 0)
    Stop();
}

void IdlenessDetector::DidProcessTask(double start_time, double end_time) {
  double task_duration = end_time - start_time;

  // Shift idle timestamps with the duration of the task, we were not idle.
  if (network_2_quiet_ > 0)
    network_2_quiet_ += task_duration;
  if (network_0_quiet_ > 0)
    network_0_quiet_ += task_duration;

  // If long task idleness detection has already been started:
  //   1) if the finished task is a long task, then restart to detect long task
  //      idleness by setting the indicator to the task end time;
  //   2) Or if the finished task is not a long task, and if there's no long
  //      task during kIdlenessWindowSeconds, emit long task idle signal.
  if (long_task_idle_ > 0) {
    if (task_duration > kLongTaskThresholdSeconds)
      long_task_idle_ = end_time;
    else if (end_time - long_task_idle_ >= kIdlenessWindowSeconds)
      EmitLongTaskIdlenessSignal();
  }
}

IdlenessDetector::IdlenessDetector(LocalFrame* local_frame)
    : local_frame_(local_frame),
      task_observer_added_(false),
      network_quiet_timer_(
          TaskRunnerHelper::Get(TaskType::kUnthrottled, local_frame),
          this,
          &IdlenessDetector::NetworkQuietTimerFired) {}

void IdlenessDetector::Stop() {
  network_quiet_timer_.Stop();
  if (!task_observer_added_)
    return;
  Platform::Current()->CurrentThread()->RemoveTaskTimeObserver(this);
  task_observer_added_ = false;
}

void IdlenessDetector::EmitLongTaskIdlenessSignal() {
  if (auto* frame_resource_coordinator =
          local_frame_->GetFrameResourceCoordinator()) {
    frame_resource_coordinator->SetProperty(
        resource_coordinator::mojom::PropertyType::kLongTaskIdle, true);
  }
  long_task_idle_ = -1;
}

void IdlenessDetector::NetworkQuietTimerFired(TimerBase*) {
  // TODO(lpy) Reduce the number of timers.
  if (network_0_quiet_ > 0 || network_2_quiet_ > 0) {
    network_quiet_timer_.StartOneShot(kNetworkQuietWatchdogSeconds,
                                      BLINK_FROM_HERE);
  }
}

DEFINE_TRACE(IdlenessDetector) {
  visitor->Trace(local_frame_);
}

}  // namespace blink
