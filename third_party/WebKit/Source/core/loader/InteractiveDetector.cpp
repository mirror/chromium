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
      network_activity_checker_(network_activity_checker) {
  // TODO(dproy): This should probably be moved to SetNavigationStartTime, and
  // we should ideally remove the observer as soon as TTCI is detected.
  Platform::Current()->CurrentThread()->AddTaskTimeObserver(this);
}

InteractiveDetector::~InteractiveDetector() {
  Platform::Current()->CurrentThread()->RemoveTaskTimeObserver(this);
}

void InteractiveDetector::SetNavigationStartTime(double navigation_start_time) {
  navigation_start_time_ = navigation_start_time;
  double initial_timer_fire_time =
      navigation_start_time + kConsistentlyInteracitveWindowSeconds;
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
  if (timer_fire_time < ci_timer_fire_time_)
    return;

  double delay = timer_fire_time - MonotonicallyIncreasingTime();
  ci_timer_fire_time_ = timer_fire_time;

  // TODO(dproy): It seems the timer never executes if delay is 0. It's possible
  // this is a bug. Hacking around the problem for now until the problem is
  // further investigated.
  if (delay <= 0.0) {
    // This argument of this function is never used and only there to fulfill
    // the API contract. nullptr should work fine.
    InteractiveTimerFired(nullptr);
  } else {
    consistently_interactive_timer_.StartOneShot(delay, BLINK_FROM_HERE);
  }
}

void InteractiveDetector::OnConsistentlyInteractiveDetected() {
  // TODO: Code to send signal to various places will go here.
  // LOG(ERROR) << "### Consistently Interactive detected ###";
  // LOG(ERROR) << "Url: " << GetSupplementable()->location()->href();
  // LOG(ERROR) << "CI time: " << consistently_interactive_;
  // LOG(ERROR) << "Navigation Start: " << navigation_start_time_;
  // LOG(ERROR) << "TTCI: " << consistently_interactive_ -
  // navigation_start_time_;
}

double InteractiveDetector::GetConsistentlyInteractiveTime() {
  return consistently_interactive_;
}

double InteractiveDetector::FindCICandidateAfterTimestamp(double timestamp) {
  std::vector<double>::iterator it;
  for (it = consistently_interactive_candidates_.begin();
       it != consistently_interactive_candidates_.end(); it++) {
    if (*it >= timestamp)
      return *it;
  }

  // Null indicating value.
  return 0.0;
}

void InteractiveDetector::CheckConsistentlyInteractiveReached() {
  if (fmp_time_ == 0.0 || dom_content_loaded_end_time_ == 0.0) {
    // FMP and DCL have not been detected yet.
    return;
  }

  double current_time = MonotonicallyIncreasingTime();
  if (current_time - fmp_time_ < kConsistentlyInteracitveWindowSeconds) {
    // Still too close to FMP.
    return;
  }

  double ci_candidate = FindCICandidateAfterTimestamp(fmp_time_);
  consistently_interactive_ =
      std::max({ci_candidate, fmp_time_, dom_content_loaded_end_time_});
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
                           kConsistentlyInteracitveWindowSeconds);
  }
}

void InteractiveDetector::DidProcessTask(double start_time, double end_time) {
  double task_length = end_time - start_time;
  if (task_length < 0.05)
    return;

  maybe_ci_candidate_ = end_time;
  StartOrPostponeCITimer(end_time + kConsistentlyInteracitveWindowSeconds);
}

void InteractiveDetector::OnFirstMeaningfulPaintDetected(double fmp_time) {
  DCHECK(fmp_time_ == 0.0);  // Should not set FMP twice.
  fmp_time_ = fmp_time;
  maybe_ci_candidate_ = std::max(maybe_ci_candidate_, fmp_time_);
  StartOrPostponeCITimer(fmp_time_ + kConsistentlyInteracitveWindowSeconds);
}

void InteractiveDetector::OnDomContentLoadedEnd(double dcl_time) {
  DCHECK(dom_content_loaded_end_time_ == 0.0);  // Should not set DCL twice.
  dom_content_loaded_end_time_ = dcl_time;
  CheckConsistentlyInteractiveReached();
}

void InteractiveDetector::InteractiveTimerFired(TimerBase*) {
  if (!GetSupplementable())
    return;

  // Value of 0.0 indicates there is currently no active timer.
  ci_timer_fire_time_ = 0.0;
  if (ActiveConnections() > kNetworkQuietMaximumConnections)
    return;

  consistently_interactive_candidates_.push_back(maybe_ci_candidate_);
  CheckConsistentlyInteractiveReached();
}

DEFINE_TRACE(InteractiveDetector) {
  Supplement<Document>::Trace(visitor);
  visitor->Trace(network_activity_checker_);
}

DEFINE_TRACE(InteractiveDetector::NetworkActivityChecker) {
  visitor->Trace(document_);
}
}  // namespace blink
