// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LeakDetector_h
#define LeakDetector_h

#include "controller/ControllerExport.h"
#include "platform/Timer.h"

namespace blink {

class Frame;
class LeakDetectorClient;
class LocalFrame;

// This class is responsible for stabilizing the detection results which are
// InstanceCounter values by 1) preparing for leak detection and 2) operating
// garbage collections before leak detection.
class CONTROLLER_EXPORT LeakDetector {
 public:
  LeakDetector(LeakDetectorClient*, LocalFrame*);
  ~LeakDetector();

  void PrepareForLeakDetection();
  void CollectGarbage();

 private:
  void TimerFiredGC(TimerBase*);

  TaskRunnerTimer<LeakDetector> delayed_gc_timer_;
  int number_of_gc_needed_;
  LeakDetectorClient* client_;
  Persistent<LocalFrame> frame_;
};
}  // namespace blink

#endif  // LeakDetector_h
