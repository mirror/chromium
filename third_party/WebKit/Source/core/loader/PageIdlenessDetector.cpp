// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/loader/PageIdlenessDetector.h"

#include "core/dom/TaskRunnerHelper.h"
#include "core/frame/LocalFrame.h"
#include "core/probe/CoreProbes.h"
#include "platform/instrumentation/resource_coordinator/FrameResourceCoordinator.h"
#include "platform/loader/fetch/ResourceFetcher.h"
#include "public/platform/Platform.h"

namespace blink {

static const char kSupplementName[] = "PageIdlenessDetector";

PageIdlenessDetector& PageIdlenessDetector::From(Document& document) {
  PageIdlenessDetector* detector = static_cast<PageIdlenessDetector*>(
      Supplement<Document>::From(document, kSupplementName));
  if (!detector) {
    detector = new PageIdlenessDetector(document);
    Supplement<Document>::ProvideTo(document, kSupplementName, detector);
  }
  return *detector;
}

void PageIdlenessDetector::DomContentLoadedEventFired() {
  network_2_quiet_ = 0;
  network_0_quiet_ = 0;

  // TODO reset frame resource coordinator.
  CheckNetworkStable();
}

// This function is called when the number of active connections is decreased.
// Note that the number of active connections doesn't decrease monotonically.
void PageIdlenessDetector::CheckNetworkStable() {
  // Document finishes parsing after DomContentLoadedEventEnd is fired,
  // check the status in order to avoid false signals.
  if (!GetSupplementable()->HasFinishedParsing())
    return;

  // If we already reported quiet time, bail out.
  if (network_0_quiet_ < 0 && network_2_quiet_ < 0)
    return;

  int request_count = GetSupplementable()->Fetcher()->ActiveRequestCount();
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

void PageIdlenessDetector::WillProcessTask(double start_time) {
  // If we have idle time and we are kNetworkQuietWindowSeconds seconds past it,
  // emit idle signals.
  if (network_2_quiet_ > 0 &&
      start_time - network_2_quiet_ > kNetworkQuietWindowSeconds) {
    probe::lifecycleEvent(GetSupplementable(), "networkAlmostIdle",
                          network_2_quiet_start_time_);
    if (auto* frame_resource_coordinator =
            GetSupplementable()->GetFrame()->GetFrameResourceCoordinator()) {
      frame_resource_coordinator->SetProperty(
          resource_coordinator::mojom::PropertyType::kNetworkAlmostIdle, true);
    }
    GetSupplementable()->Fetcher()->OnNetworkQuiet();
    network_2_quiet_ = -1;
  }

  if (network_0_quiet_ > 0 &&
      start_time - network_0_quiet_ > kNetworkQuietWindowSeconds) {
    probe::lifecycleEvent(GetSupplementable(), "networkIdle",
                          network_0_quiet_start_time_);
    network_0_quiet_ = -1;
  }
}

void PageIdlenessDetector::DidProcessTask(double start_time, double end_time) {
  // Shift idle timestamps with the duration of the task, we were not idle.
  if (network_2_quiet_ > 0)
    network_2_quiet_ += end_time - start_time;
  if (network_0_quiet_ > 0)
    network_0_quiet_ += end_time - start_time;
}

PageIdlenessDetector::PageIdlenessDetector(Document& document)
    : Supplement<Document>(document),
      network_quiet_timer_(
          TaskRunnerHelper::Get(TaskType::kUnthrottled, &document),
          this,
          &PageIdlenessDetector::NetworkQuietTimerFired) {
  Platform::Current()->CurrentThread()->AddTaskTimeObserver(this);
}

PageIdlenessDetector::~PageIdlenessDetector() {
  network_quiet_timer_.Stop();
  Platform::Current()->CurrentThread()->RemoveTaskTimeObserver(this);
}

void PageIdlenessDetector::NetworkQuietTimerFired(TimerBase*) {
  if (network_0_quiet_ > 0 || network_2_quiet_ > 0) {
    network_quiet_timer_.StartOneShot(kNetworkQuietWatchdogSeconds,
                                      BLINK_FROM_HERE);
  }
}

DEFINE_TRACE(PageIdlenessDetector) {
  Supplement<Document>::Trace(visitor);
}

}  // namespace blink
