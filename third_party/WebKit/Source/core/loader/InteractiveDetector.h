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
class ExecutionContext;

class CORE_EXPORT InteractiveDetector
    : public GarbageCollectedFinalized<InteractiveDetector>,
      public Supplement<Document>,
      public scheduler::TaskTimeObserver {
  WTF_MAKE_NONCOPYABLE(InteractiveDetector);
  USING_GARBAGE_COLLECTED_MIXIN(InteractiveDetector);

 public:
  class NetworkActivityChecker
      : public GarbageCollected<NetworkActivityChecker> {
    WTF_MAKE_NONCOPYABLE(NetworkActivityChecker);

   public:
    NetworkActivityChecker(Document* document) : document_(document) {}

    virtual int GetActiveConnections();

   private:
    Member<Document> document_;

    DECLARE_VIRTUAL_TRACE();
  };

  static InteractiveDetector* From(Document&);
  virtual ~InteractiveDetector();

  void OnResourceLoadEnd(double load_finish_time);
  void SetNavigationStartTime(double navigation_start_time);
  void OnFirstMeaningfulPaintDetected(double fmp_time);
  void OnDomContentLoadedEnd(double dcl_time);
  void OnConsistentlyInteractiveDetected();

  // Returns 0.0 to indicate consistently interactive has not been detected yet.
  double GetConsistentlyInteractiveTime();

  DECLARE_VIRTUAL_TRACE();

 private:
  friend class InteractiveDetectorTest;

  // The page is quiet if there are no more than 2 active network requests for
  // this duration of time.
  static constexpr double kConsistentlyInteracitveWindowSeconds = 5.0;
  static constexpr int kNetworkQuietMaximumConnections = 2;

  explicit InteractiveDetector(Document&, NetworkActivityChecker*);
  int ActiveConnections();
  void InteractiveTimerFired(TimerBase*);
  void StartOrPostponeCITimer(double timer_fire_time);
  void CheckConsistentlyInteractiveReached();
  double FindCICandidateAfterTimestamp(double timestamp);
  double fmp_time_ = 0.0;
  double dom_content_loaded_end_time_ = 0.0;
  double consistently_interactive_ = 0.0;
  double maybe_ci_candidate_ = 0.0;
  double ci_timer_fire_time_ = 0.0;
  double navigation_start_time_ = 0.0;
  std::vector<double> consistently_interactive_candidates_;
  TaskRunnerTimer<InteractiveDetector> consistently_interactive_timer_;
  Member<NetworkActivityChecker> network_activity_checker_;

  // scheduler::TaskTimeObserver implementation
  void WillProcessTask(double start_time) override {}
  void DidProcessTask(double start_time, double end_time) override;
  void OnBeginNestedRunLoop() override {}
  void WillExecuteScript(ExecutionContext*) {}
  void DidExecuteScript() {}
};

}  // namespace blink

#endif
