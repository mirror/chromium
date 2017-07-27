// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LeakDetector_h
#define LeakDetector_h

#include "core/CoreExport.h"
#include "core/frame/LocalFrame.h"
#include "platform/Timer.h"

namespace blink {

class LeakDetectorClient {
 public:
  virtual void OnLeakDetectionComplete() = 0;
};

class CORE_EXPORT LeakDetector {
 public:
  explicit LeakDetector(LeakDetectorClient*);
  ~LeakDetector();

  void PrepareForLeakDetection(LocalFrame*);
  void CollectGarbage();

  typedef void (*AbstractAnimationWorkletThreadGCFunction)(void);
  static void InitAbstractAnimationWorkletThreadGC(
      AbstractAnimationWorkletThreadGCFunction);

 private:
  void DelayedGC(TimerBase*);
  void Report(TimerBase*);

  void ExecuteAbstractAnimationWorkletThreadGC();

  TaskRunnerTimer<LeakDetector> delayed_gc_timer_;
  TaskRunnerTimer<LeakDetector> delayed_report_timer_;
  int number_of_gc_needed_;
  LeakDetectorClient* client_;
};
}  // namespace blink

#endif  // LeakDetector_h
