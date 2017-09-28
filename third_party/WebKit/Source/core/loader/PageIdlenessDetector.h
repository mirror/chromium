// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PageIdlenessDetector_h
#define PageIdlenessDetector_h

#include "core/CoreExport.h"
#include "core/dom/Document.h"
#include "platform/Supplementable.h"
#include "platform/Timer.h"
#include "platform/heap/Handle.h"
#include "platform/scheduler/base/task_time_observer.h"
#include "platform/wtf/Noncopyable.h"

namespace blink {

class Document;

// PageIdlenessDetector observes network request count everytime a load is
// finshed after DOMContentLoadedEventEnd is fired, and signals a network
// idleness signal to GRC when there are no more than 2 network connection
// active in 1 second.
class CORE_EXPORT PageIdlenessDetector
    : public GarbageCollectedFinalized<PageIdlenessDetector>,
      public scheduler::TaskTimeObserver,
      public Supplement<Document> {
  WTF_MAKE_NONCOPYABLE(PageIdlenessDetector);
  USING_GARBAGE_COLLECTED_MIXIN(PageIdlenessDetector);

 public:
  static PageIdlenessDetector& From(Document&);
  virtual ~PageIdlenessDetector();

  void DomContentLoadedEventFired();
  void CheckNetworkStable();

  DECLARE_VIRTUAL_TRACE();

 private:
  friend class PageIdlenessDetectorTest;

  // The page is quiet if there are no more than 2 active network requests for
  // this duration of time.
  static constexpr double kNetworkQuietWindowSeconds = 0.5;
  static constexpr double kNetworkQuietWatchdogSeconds = 2;
  static constexpr int kNetworkQuietMaximumConnections = 2;

  // scheduler::TaskTimeObserver implementation
  void WillProcessTask(double start_time) override;
  void DidProcessTask(double start_time, double end_time) override;

  explicit PageIdlenessDetector(Document&);
  void NetworkQuietTimerFired(TimerBase*);

  // Store the accumulated time of network quiet.
  double network_0_quiet_ = 0;
  double network_2_quiet_ = 0;
  // Record the actual start time of network quiet.
  double network_0_quiet_start_time_;
  double network_2_quiet_start_time_;
  TaskRunnerTimer<PageIdlenessDetector> network_quiet_timer_;
};

}  // namespace blink

#endif
