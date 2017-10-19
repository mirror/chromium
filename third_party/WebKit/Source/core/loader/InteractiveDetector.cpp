// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/loader/InteractiveDetector.h"

#include "core/dom/Document.h"
#include "core/dom/TaskRunnerHelper.h"
#include "platform/instrumentation/tracing/TraceEvent.h"
#include "platform/loader/fetch/ResourceFetcher.h"
#include "platform/wtf/CurrentTime.h"

namespace blink {

static constexpr char kSupplementName[] = "InteractiveDetector";

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
      network_activity_checker_(network_activity_checker),
      consistently_interactive_timer_(
          TaskRunnerHelper::Get(TaskType::kUnspecedTimer, &document),
          this,
          &InteractiveDetector::ConsistentlyInteractiveTimerFired) {}

InteractiveDetector::~InteractiveDetector() {
  LongTaskDetector::Instance().UnregisterObserver(this);
}

void InteractiveDetector::SetNavigationStartTime(double navigation_start_time) {
  // Should not set nav start twice.
  DCHECK(page_event_times_.nav_start == 0.0);

  LongTaskDetector::Instance().RegisterObserver(this);
  page_event_times_.nav_start = navigation_start_time;
  double initial_timer_fire_time =
      navigation_start_time + kConsistentlyInteractiveWindowSeconds;

  active_main_thread_quiet_window_start_ = navigation_start_time;
  active_network_quiet_window_start_ = navigation_start_time;
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
  // This function should never be called after Consistently Interactive is
  // reached.
  DCHECK(consistently_interactive_ == 0.0);

  // We give 1ms extra padding to the timer fire time to prevent floating point
  // arithmetic pitfalls when comparing window sizes.
  timer_fire_time += 0.001;

  // Return if there is an active timer scheduled to fire later than
  // |timer_fire_time|.
  if (timer_fire_time < consistently_interactive_timer_fire_time_)
    return;

  double delay = timer_fire_time - MonotonicallyIncreasingTime();
  consistently_interactive_timer_fire_time_ = timer_fire_time;

  if (delay <= 0.0) {
    // This argument of this function is never used and only there to fulfill
    // the API contract. nullptr should work fine.
    ConsistentlyInteractiveTimerFired(nullptr);
  } else {
    consistently_interactive_timer_.StartOneShot(delay, BLINK_FROM_HERE);
  }
}

void InteractiveDetector::OnConsistentlyInteractiveDetected() {
  LongTaskDetector::Instance().UnregisterObserver(this);
  main_thread_quiet_windows_.clear();
  network_quiet_windows_.clear();

  TRACE_EVENT_MARK_WITH_TIMESTAMP1(
      "loading,rail", "consistentlyInteractive",
      TraceEvent::ToTraceTimestamp(consistently_interactive_), "frame",
      GetSupplementable()->GetFrame());
}

double InteractiveDetector::GetConsistentlyInteractiveTime() {
  return consistently_interactive_;
}

void InteractiveDetector::BeginNetworkQuietPeriod(double current_time) {
  // Value of 0.0 indicates there is no currently actively network quiet window.
  DCHECK(active_network_quiet_window_start_ == 0.0);
  active_network_quiet_window_start_ = current_time;

  StartOrPostponeCITimer(current_time + kConsistentlyInteractiveWindowSeconds);
}

void InteractiveDetector::EndNetworkQuietPeriod(double current_time) {
  DCHECK(active_network_quiet_window_start_ != 0.0);

  if (current_time - active_network_quiet_window_start_ >=
      kConsistentlyInteractiveWindowSeconds) {
    network_quiet_windows_.emplace_back(active_network_quiet_window_start_,
                                        current_time);
  }
  active_network_quiet_window_start_ = 0.0;
}

// The optional opt_current_time, if provided, saves us a call to
// MonotonicallyIncreasingTime.
void InteractiveDetector::UpdateNetworkQuietState(
    double request_count,
    WTF::Optional<double> opt_current_time) {
  if (request_count <= kNetworkQuietMaximumConnections &&
      active_network_quiet_window_start_ == 0.0) {
    // Not using `value_or(MonotonicallyIncreasingTime())` here because
    // arguments to functions are eagerly evaluated, which always call
    // MonotonicallyIncreasingTime.
    double current_time = opt_current_time ? opt_current_time.value()
                                           : MonotonicallyIncreasingTime();
    BeginNetworkQuietPeriod(current_time);
  } else if (request_count > kNetworkQuietMaximumConnections &&
             active_network_quiet_window_start_ != 0.0) {
    double current_time = opt_current_time ? opt_current_time.value()
                                           : MonotonicallyIncreasingTime();
    EndNetworkQuietPeriod(current_time);
  }
}

void InteractiveDetector::OnResourceLoadBegin(
    WTF::Optional<double> load_begin_time) {
  if (consistently_interactive_ != 0.0)
    return;
  // The request that is about to begin is not counted in ActiveConnections(),
  // so we add one to it.
  UpdateNetworkQuietState(ActiveConnections() + 1, load_begin_time);
}

// The optional load_finish_time, if provided, saves us a call to
// MonotonicallyIncreasingTime.
void InteractiveDetector::OnResourceLoadEnd(
    WTF::Optional<double> load_finish_time) {
  if (consistently_interactive_ != 0.0)
    return;
  UpdateNetworkQuietState(ActiveConnections(), load_finish_time);
}

void InteractiveDetector::OnLongTaskDetected(double start_time,
                                             double end_time) {
  // We should not be receiving long task notifications after Consistently
  // Interactive has already been reached.
  DCHECK(consistently_interactive_ == 0.0);
  double quiet_window_length =
      start_time - active_main_thread_quiet_window_start_;
  if (quiet_window_length >= kConsistentlyInteractiveWindowSeconds) {
    main_thread_quiet_windows_.emplace_back(
        active_main_thread_quiet_window_start_, start_time);
  }
  active_main_thread_quiet_window_start_ = end_time;
  StartOrPostponeCITimer(end_time + kConsistentlyInteractiveWindowSeconds);
}

void InteractiveDetector::OnFirstMeaningfulPaintDetected(double fmp_time) {
  DCHECK(page_event_times_.fmp == 0.0);  // Should not set FMP twice.
  page_event_times_.fmp = fmp_time;
  if (MonotonicallyIncreasingTime() - fmp_time >=
      kConsistentlyInteractiveWindowSeconds) {
    // We may have reached TTCI already. Check right away.
    CheckConsistentlyInteractiveReached();
  } else {
    StartOrPostponeCITimer(page_event_times_.fmp +
                           kConsistentlyInteractiveWindowSeconds);
  }
}

void InteractiveDetector::OnDomContentLoadedEnd(double dcl_end_time) {
  DCHECK(page_event_times_.dcl_end == 0.0);  // Should not set DCL twice.
  page_event_times_.dcl_end = dcl_end_time;
  CheckConsistentlyInteractiveReached();
}

void InteractiveDetector::ConsistentlyInteractiveTimerFired(TimerBase*) {
  if (!GetSupplementable() || consistently_interactive_ != 0.0)
    return;

  // Value of 0.0 indicates there is currently no active timer.
  consistently_interactive_timer_fire_time_ = 0.0;
  CheckConsistentlyInteractiveReached();
}

void InteractiveDetector::AddCurrentlyActiveQuietIntervals(
    double current_time) {
  // Network is currently quiet.
  if (active_network_quiet_window_start_ != 0.0) {
    if (current_time - active_network_quiet_window_start_ >=
        kConsistentlyInteractiveWindowSeconds) {
      network_quiet_windows_.emplace_back(active_network_quiet_window_start_,
                                          current_time);
    }
  }

  // Since this code executes on the main thread, we know that no task is
  // currently running on the main thread. We can therefore skip checking.
  // main_thread_quiet_window_being != 0.0.
  if (current_time - active_main_thread_quiet_window_start_ >=
      kConsistentlyInteractiveWindowSeconds) {
    main_thread_quiet_windows_.emplace_back(
        active_main_thread_quiet_window_start_, current_time);
  }
}

void InteractiveDetector::RemoveCurrentlyActiveQuietIntervals() {
  if (!network_quiet_windows_.empty() &&
      network_quiet_windows_.back().start ==
          active_network_quiet_window_start_) {
    network_quiet_windows_.pop_back();
  }

  if (!main_thread_quiet_windows_.empty() &&
      main_thread_quiet_windows_.back().start ==
          active_main_thread_quiet_window_start_) {
    main_thread_quiet_windows_.pop_back();
  }
}

double InteractiveDetector::FindConsistentlyInteractiveCandidate(
    double lower_bound) {
  // Main thread iterator.
  auto it_mt = main_thread_quiet_windows_.begin();
  // Network iterator.
  auto it_net = network_quiet_windows_.begin();

  while (it_mt < main_thread_quiet_windows_.end() &&
         it_net < network_quiet_windows_.end()) {
    if (it_mt->end <= lower_bound) {
      it_mt++;
      continue;
    }
    if (it_net->end <= lower_bound) {
      it_net++;
      continue;
    }

    // First handling the no overlap cases.
    // [ main thread interval ]
    //                                     [ network interval ]
    if (it_mt->end <= it_net->start) {
      it_mt++;
      continue;
    }
    //                                     [ main thread interval ]
    // [   network interval   ]
    if (it_net->end <= it_mt->start) {
      it_net++;
      continue;
    }

    // At this point we know we have a non-empty overlap after lower_bound.
    double overlap_start = std::max({it_mt->start, it_net->start, lower_bound});
    double overlap_end = std::min(it_mt->end, it_net->end);
    double overlap_duration = overlap_end - overlap_start;
    if (overlap_duration >= kConsistentlyInteractiveWindowSeconds) {
      return std::max(lower_bound, it_mt->start);
    }

    // The interval with earlier end time will not produce any more overlap, so
    // we move on from it.
    if (it_mt->end <= it_net->end) {
      it_mt++;
    } else {
      it_net++;
    }
  }

  // Consistently Interactive candidate not found.
  return 0.0;
}

void InteractiveDetector::CheckConsistentlyInteractiveReached() {
  // Already detected consistently interactive.
  if (consistently_interactive_ != 0.0)
    return;

  // FMP and DCL have not been detected yet.
  if (page_event_times_.fmp == 0.0 || page_event_times_.dcl_end == 0.0)
    return;

  const double current_time = MonotonicallyIncreasingTime();
  if (current_time - page_event_times_.fmp <
      kConsistentlyInteractiveWindowSeconds) {
    // Too close to FMP to determine Consistently Interactive.
    return;
  }

  AddCurrentlyActiveQuietIntervals(current_time);
  const double consistently_interactive_candidate =
      FindConsistentlyInteractiveCandidate(page_event_times_.fmp);
  RemoveCurrentlyActiveQuietIntervals();

  // No Consistently Interactive Found.
  if (consistently_interactive_candidate == 0.0)
    return;

  consistently_interactive_ =
      std::max({consistently_interactive_candidate, page_event_times_.dcl_end});
  OnConsistentlyInteractiveDetected();
}

DEFINE_TRACE(InteractiveDetector) {
  Supplement<Document>::Trace(visitor);
}

}  // namespace blink
