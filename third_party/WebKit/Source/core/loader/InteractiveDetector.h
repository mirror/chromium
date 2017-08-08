// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef InteractiveDetector_h
#define InteractiveDetector_h

#include "core/CoreExport.h"
#include "platform/Supplementable.h"
#include "platform/Timer.h"
#include "platform/heap/Handle.h"
#include "platform/scheduler/base/task_time_observer.h"
#include "platform/wtf/Noncopyable.h"

namespace blink {

class Document;

class CORE_EXPORT InteractiveDetector
    : public GarbageCollectedFinalized<InteractiveDetector>,
      public Supplement<Document>,
      public scheduler::TaskTimeObserver {
  WTF_MAKE_NONCOPYABLE(InteractiveDetector);
  USING_GARBAGE_COLLECTED_MIXIN(InteractiveDetector);

 public:
  // This class can be easily switched out to allow better testing of
  // InteractiveDetector.
  class NetworkActivityChecker {
    WTF_MAKE_NONCOPYABLE(NetworkActivityChecker);

   public:
    NetworkActivityChecker(Document* document) : document_(document) {}

    virtual int GetActiveConnections();

   private:
    WeakPersistent<Document> document_;
  };

  static InteractiveDetector* From(Document&);
  virtual ~InteractiveDetector();

  void OnResourceLoadEnd(double load_finish_time);
  void SetNavigationStartTime(double navigation_start_time);
  void OnFirstMeaningfulPaintDetected(double fmp_time);
  void OnDomContentLoadedEnd(double dcl_time);

  // Returns 0.0 to indicate consistently interactive has not been detected yet.
  double GetConsistentlyInteractiveTime();

  DECLARE_VIRTUAL_TRACE();

 private:
  friend class InteractiveDetectorTest;

  // The page is quiet if there are no more than 2 active network requests for
  // this duration of time.
  static constexpr double kConsistentlyInteractiveWindowSeconds = 5.0;
  static constexpr int kNetworkQuietMaximumConnections = 2;

  explicit InteractiveDetector(Document&, NetworkActivityChecker*);
  int ActiveConnections();
  void InteractiveTimerFired(TimerBase*);
  void StartOrPostponeCITimer(double timer_fire_time);
  void CheckConsistentlyInteractiveReached();
  double FindCICandidateAfterTimestamp(double timestamp);
  void OnConsistentlyInteractiveDetected();

  // Page event times that Interactive Detector depends on.
  // Value of 0.0 indicate the event has not been detected yet.
  struct {
    double fmp = 0.0;
    double dcl_end = 0.0;
    double nav_start = 0.0;
  } page_event_times_;

  double consistently_interactive_ = 0.0;
  double main_thread_quiet_window_begin_ = 0.0;
  double consistently_interactive_timer_fire_time_ = 0.0;
  std::vector<double> consistently_interactive_candidates_;
  TaskRunnerTimer<InteractiveDetector> consistently_interactive_timer_;
  std::unique_ptr<NetworkActivityChecker> network_activity_checker_;

  // scheduler::TaskTimeObserver implementation
  void WillProcessTask(double start_time) override {}
  void DidProcessTask(double start_time, double end_time) override;
  void OnBeginNestedRunLoop() override {}
};

}  // namespace blink

#endif
