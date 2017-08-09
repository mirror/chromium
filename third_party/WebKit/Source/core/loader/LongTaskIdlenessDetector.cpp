// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/loader/LongTaskIdlenessDetector.h"
#include "core/dom/Document.h"
#include "core/dom/TaskRunnerHelper.h"
#include "core/frame/LocalFrame.h"
#include "platform/instrumentation/resource_coordinator/FrameResourceCoordinator.h"
#include "public/platform/Platform.h"

namespace blink {

static const char kSupplementName[] = "LongTaskIdlenessDetector";

LongTaskIdlenessDetector& LongTaskIdlenessDetector::From(Document* document) {
  DCHECK(document->IsInMainFrame());
  LongTaskIdlenessDetector* detector = static_cast<LongTaskIdlenessDetector*>(
      Supplement<Document>::From(*document, kSupplementName));
  if (!detector) {
    detector = new LongTaskIdlenessDetector(document);
    Supplement<Document>::ProvideTo(*document, kSupplementName, detector);
  }
  return *detector;
}

void LongTaskIdlenessDetector::Init() {
  Platform::Current()->CurrentThread()->AddTaskTimeObserver(this);
}

LongTaskIdlenessDetector::LongTaskIdlenessDetector(Document* document)
    : Supplement<Document>(*document),
      long_task_idleness_timer_(
          TaskRunnerHelper::Get(TaskType::kUnspecedTimer, document),
          this,
          &LongTaskIdlenessDetector::LongTaskIdlenessTimerFired) {
  DCHECK(document->IsInMainFrame());
}

LongTaskIdlenessDetector::~LongTaskIdlenessDetector() {
  if (!long_task_idleness_reached_) {
    Platform::Current()->CurrentThread()->RemoveTaskTimeObserver(this);
  }
}

void LongTaskIdlenessDetector::DidProcessTask(double start_time,
                                              double end_time) {
  static double long_task_duration = 0.05;

  // Document finishes parsing after DomContentLoadedEventEnd is fired,
  // check the status in order to avoid false signals.
  if (long_task_idleness_reached_ || !GetSupplementable()->HasFinishedParsing())
    return;

  if (end_time - start_time < long_task_duration)
    return;

  // A long task is detected, restart timer.
  long_task_idleness_timer_.StartOneShot(kLongTaskIdlenessWindowSeconds,
                                         BLINK_FROM_HERE);
}

void LongTaskIdlenessDetector::LongTaskIdlenessTimerFired(TimerBase*) {
  if (!GetSupplementable() || !GetSupplementable()->GetFrame() ||
      long_task_idleness_reached_)
    return;

  long_task_idleness_reached_ = true;
  Platform::Current()->CurrentThread()->RemoveTaskTimeObserver(this);
  // TODO(lpy): Report to FrameCU
}

DEFINE_TRACE(LongTaskIdlenessDetector) {
  Supplement<Document>::Trace(visitor);
}

}  // namespace blink
