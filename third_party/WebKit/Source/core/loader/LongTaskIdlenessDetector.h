// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LongTaskIdlenessDetector_h
#define LongTaskIdlenessDetector_h

#include "core/CoreExport.h"
#include "platform/Supplementable.h"
#include "platform/Timer.h"
#include "platform/heap/Handle.h"
#include "platform/scheduler/base/task_time_observer.h"
#include "platform/wtf/Noncopyable.h"

namespace blink {

class Document;

// LongTaskIdlenessDetector is essentailly a task time observer, it will sends
// long task idleness signal out when there is a 0.5 second window that has no
// long task.
class CORE_EXPORT LongTaskIdlenessDetector
    : public GarbageCollectedFinalized<LongTaskIdlenessDetector>,
      public Supplement<Document>,
      public scheduler::TaskTimeObserver {
  WTF_MAKE_NONCOPYABLE(LongTaskIdlenessDetector);
  USING_GARBAGE_COLLECTED_MIXIN(LongTaskIdlenessDetector);

 public:
  static LongTaskIdlenessDetector& From(Document*);

  virtual ~LongTaskIdlenessDetector();
  void Init();

  DECLARE_VIRTUAL_TRACE();

 private:
  // The page is considered long task idleness if there is no long task in 0.5
  // second.
  static constexpr double kLongTaskIdlenessWindowSeconds = 0.5;

  explicit LongTaskIdlenessDetector(Document*);

  // scheduler::TaskTimeObserver implementation.
  void WillProcessTask(double start_time) override {}
  void DidProcessTask(double start_time, double end_time) override;
  void OnBeginNestedRunLoop() override {}

  void LongTaskIdlenessTimerFired(TimerBase*);

  bool long_task_idleness_reached_ = false;
  TaskRunnerTimer<LongTaskIdlenessDetector> long_task_idleness_timer_;
};

}  // namespace blink

#endif  // LongTaskIdlenessDetector_h
