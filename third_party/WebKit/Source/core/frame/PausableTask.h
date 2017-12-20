// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PausableTask_h
#define PausableTask_h

#include <memory>

#include "core/CoreExport.h"
#include "core/frame/PausableTimer.h"
#include "platform/heap/SelfKeepAlive.h"
#include "public/web/WebLocalFrame.h"

namespace blink {

// A class that monitors the lifetime and suspension state of a context, and
// runs a task either when the context is unsuspended or when the context is
// invalidated.
class CORE_EXPORT PausableTask final
    : public GarbageCollectedFinalized<PausableTask>,
      public PausableTimer {
  USING_GARBAGE_COLLECTED_MIXIN(PausableTask);

 public:
  // Note: This asserts that the context is currently suspended.
  PausableTask(ExecutionContext*, WebLocalFrame::PausableTaskCallback);
  ~PausableTask() override;

  // PausableTimer:
  void ContextDestroyed(ExecutionContext*) override;
  void Fired() override;

 private:
  void Dispose();

  WebLocalFrame::PausableTaskCallback callback_;

  SelfKeepAlive<PausableTask> keep_alive_;
};

}  // namespace blink

#endif  // PausableTask_h
