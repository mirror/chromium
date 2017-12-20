// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/frame/PausableTask.h"

#include "core/dom/Document.h"

namespace blink {

PausableTask::PausableTask(ExecutionContext* context,
                           WebLocalFrame::PausableTaskCallback callback)
    : PausableTimer(context, TaskType::kJavascriptTimer),
      callback_(std::move(callback)),
      keep_alive_(this) {
  DCHECK(callback_);
  DCHECK(context);
  DCHECK(!context->IsContextDestroyed());
  DCHECK(context->IsContextPaused());

  StartOneShot(TimeDelta(), BLINK_FROM_HERE);
  PauseIfNeeded();
}

PausableTask::~PausableTask() {}

void PausableTask::ContextDestroyed(ExecutionContext* destroyed_context) {
  PausableTimer::ContextDestroyed(destroyed_context);
  DCHECK(callback_);
  std::move(callback_).Run(
      WebLocalFrame::PausableTaskResult::kContextInvalidOrDestroyed);
  Dispose();
}

void PausableTask::Fired() {
  CHECK(!GetExecutionContext()->IsContextDestroyed());
  DCHECK(!GetExecutionContext()->IsContextPaused());
  DCHECK(callback_);

  auto callback = std::move(callback_);

  // Call Dispose() now, since it's possible that the callback will destroy the
  // context.
  Dispose();

  std::move(callback).Run(WebLocalFrame::PausableTaskResult::kReady);
}

void PausableTask::Dispose() {
  // Remove object as a ContextLifecycleObserver.
  PausableObject::ClearContext();
  keep_alive_.Clear();
  Stop();
}

}  // namespace blink
