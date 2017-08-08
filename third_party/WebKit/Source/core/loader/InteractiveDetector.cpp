// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/loader/InteractiveDetector.h"

#include "core/dom/Document.h"
#include "core/dom/TaskRunnerHelper.h"
#include "core/loader/DocumentLoader.h"
#include "platform/loader/fetch/ResourceFetcher.h"
#include "platform/wtf/CurrentTime.h"
#include "public/platform/Platform.h"

// Only for debugging logs. Will remove before landing.
#include "core/frame/Location.h"

namespace blink {

static const char kSupplementName[] = "InteractiveDetector";

InteractiveDetector* InteractiveDetector::From(Document& document) {
  InteractiveDetector* detector = static_cast<InteractiveDetector*>(
      Supplement<Document>::From(document, kSupplementName));
  if (!detector) {
    if (!document.IsInMainFrame()) {
      return nullptr;
    }
    detector = new InteractiveDetector(document,
                                       new NetworkActivityChecker(&document));
    Supplement<Document>::ProvideTo(document, kSupplementName, detector);
  }
  return detector;
}

InteractiveDetector::InteractiveDetector(
    Document& document,
    NetworkActivityChecker* network_activity_checker)
    : Supplement<Document>(document),
      consistently_interactive_timer_(
          TaskRunnerHelper::Get(TaskType::kUnspecedTimer, &document),
          this,
          &InteractiveDetector::InteractiveTimerFired),
      network_activity_checker_(network_activity_checker) {}

InteractiveDetector::~InteractiveDetector() {
  Platform::Current()->CurrentThread()->RemoveTaskTimeObserver(this);
}

void InteractiveDetector::SetNavigationStartTime(double navigation_start_time) {
  // Should not set nav start twice.
  DCHECK(page_event_times_.nav_start == 0.0);

  Platform::Current()->CurrentThread()->AddTaskTimeObserver(this);
  page_event_times_.nav_start = navigation_start_time;
  double initial_timer_fire_time =
      navigation_start_time + kConsistentlyInteractiveWindowSeconds;
  main_thread_quiet_window_begin_ = navigation_start_time;
  StartOrPostponeCITimer(initial_timer_fire_time);
}

int InteractiveDetector::NetworkActivityChecker::GetActiveConnections() {
  ResourceFetcher* fetcher = document_->Fetcher();
  return fetcher->BlockingRequestCount() + fetcher->NonblockingRequestCount();
}

int InteractiveDetector::ActiveConnections() {
  return network_activity_checker_->GetActiveConnections();
}

void InteractiveDetector::StartOrPostponeCITimer(double timer_fire_time) {
  // Return if already detected Consistently Interactive.
  if (consistently_interactive_)
    return;

  // Return if there is an active timer scheduled to fire later than
  // |timer_fire_time|.
  if (timer_fire_time < consistently_interactive_timer_fire_time_)
    return;

  double delay = timer_fire_time - MonotonicallyIncreasingTime();
  consistently_interactive_timer_fire_time_ = timer_fire_time;

  if (delay <= 0.0) {
    // This argument of this function is never used and only there to fulfill
    // the API contract. nullptr should work fine.
    InteractiveTimerFired(nullptr);
  } else {
    consistently_interactive_timer_.StartOneShot(delay, BLINK_FROM_HERE);
  }
}

void InteractiveDetector::OnConsistentlyInteractiveDetected() {
  Platform::Current()->CurrentThread()->RemoveTaskTimeObserver(this);
  // TODO: Code to send signal to various places will go here.
  // LOG(ERROR) << "### Consistently Interactive detected ###";
  // LOG(ERROR) << "Url: " << GetSupplementable()->location()->href();
  // LOG(ERROR) << "CI time: " << consistently_interactive_;
  // LOG(ERROR) << "Navigation Start: " << page_event_times_.nav_start;
  // LOG(ERROR) << "TTCI: " << consistently_interactive_ -
  // page_event_times_.nav_start;
}

double InteractiveDetector::GetConsistentlyInteractiveTime() {
  return consistently_interactive_;
}

double InteractiveDetector::FindCICandidateAfterTimestamp(double timestamp) {
  for (auto& ci_candidate : consistently_interactive_candidates_) {
    if (ci_candidate >= timestamp)
      return ci_candidate;
  }

  // Null indicating value.
  return 0.0;
}

void InteractiveDetector::CheckConsistentlyInteractiveReached() {
  if (page_event_times_.fmp == 0.0 || page_event_times_.dcl_end == 0.0) {
    // FMP and DCL have not been detected yet.
    return;
  }

  double current_time = MonotonicallyIncreasingTime();
  if (current_time - page_event_times_.fmp <
      kConsistentlyInteractiveWindowSeconds) {
    // Still too close to FMP.
    return;
  }

  double first_ci_candidate_after_fmp =
      FindCICandidateAfterTimestamp(page_event_times_.fmp);
  consistently_interactive_ =
      std::max({first_ci_candidate_after_fmp, page_event_times_.fmp,
                page_event_times_.dcl_end});
  OnConsistentlyInteractiveDetected();
}

void InteractiveDetector::OnResourceLoadEnd(double load_finish_time) {
  int active_connections = ActiveConnections();

  // At each OnResourceLoadEnd call, active connections for this document has
  // decreased by exactly one. Therefore if active_connections ==
  // kNetworkQuietMaximumConnections, this means active_conenctions was equal to
  // kNetworkQuietMaximumConnections + 1 moments ago and we have just become
  // network quiet.
  if (active_connections == kNetworkQuietMaximumConnections) {
    StartOrPostponeCITimer(load_finish_time +
                           kConsistentlyInteractiveWindowSeconds);
  }
}

void InteractiveDetector::DidProcessTask(double start_time, double end_time) {
  double task_length = end_time - start_time;
  if (task_length < 0.05)
    return;

  main_thread_quiet_window_begin_ = end_time;
  StartOrPostponeCITimer(end_time + kConsistentlyInteractiveWindowSeconds);
}

void InteractiveDetector::OnFirstMeaningfulPaintDetected(double fmp_time) {
  DCHECK(page_event_times_.fmp == 0.0);  // Should not set FMP twice.
  page_event_times_.fmp = fmp_time;
  StartOrPostponeCITimer(page_event_times_.fmp +
                         kConsistentlyInteractiveWindowSeconds);
}

void InteractiveDetector::OnDomContentLoadedEnd(double dcl_end_time) {
  DCHECK(page_event_times_.dcl_end == 0.0);  // Should not set DCL twice.
  page_event_times_.dcl_end = dcl_end_time;
  CheckConsistentlyInteractiveReached();
}

void InteractiveDetector::InteractiveTimerFired(TimerBase*) {
  if (!GetSupplementable())
    return;

  // Value of 0.0 indicates there is currently no active timer.
  consistently_interactive_timer_fire_time_ = 0.0;
  if (ActiveConnections() > kNetworkQuietMaximumConnections)
    return;

  consistently_interactive_candidates_.push_back(
      main_thread_quiet_window_begin_);
  CheckConsistentlyInteractiveReached();
}

DEFINE_TRACE(InteractiveDetector) {
  Supplement<Document>::Trace(visitor);
}

}  // namespace blink
