// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LeakDetector_h
#define LeakDetector_h

#include "core/CoreExport.h"
#include "platform/Timer.h"

namespace blink {

class Frame;
class LeakDetectorClient;

// This class is responsible for stabilizing the detection results which are
// InstanceCounter values by 1) preparing for leak detection and 2) operating
// garbage collections before leak detection.
class CORE_EXPORT LeakDetector {
 public:
  explicit LeakDetector(LeakDetectorClient*);
  ~LeakDetector();

  void PrepareForLeakDetection(Frame*);
  void CollectGarbage();

  typedef void (*AbstractAnimationWorkletThreadGCFunction)(void);
  static void InitAbstractAnimationWorkletThreadGC(
      AbstractAnimationWorkletThreadGCFunction);

 private:
  void TimerFiredGC(TimerBase*);

  void ExecuteAbstractAnimationWorkletThreadGC();

  TaskRunnerTimer<LeakDetector> delayed_gc_timer_;
  int number_of_gc_needed_;
  LeakDetectorClient* client_;
};
}  // namespace blink

#endif  // LeakDetector_h
