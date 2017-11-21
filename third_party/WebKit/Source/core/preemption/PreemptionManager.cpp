// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/preemption/PreemptionManager.h"

#include "base/run_loop.h"
#include "core/frame/LocalFrame.h"
#include "platform/WebFrameScheduler.h"
#include "platform/instrumentation/tracing/TraceEvent.h"
#include "v8/include/v8.h"

namespace blink {
namespace {

constexpr WTF::TimeDelta kNestedLoopMinimumInterval =
    WTF::TimeDelta::FromMilliseconds(100);

class LoopSuspensionScope {
 public:
  explicit LoopSuspensionScope(v8::Isolate* isolate)
      : suppress_microtasks_(isolate) {
    v8::Local<v8::Context> context = isolate->GetCurrentContext();
    ScriptState* script_state = ScriptState::From(context);
    ExecutionContext* execution_context = ExecutionContext::From(script_state);
    if (!execution_context->IsDocument())
      return;
    Document* document = static_cast<Document*>(execution_context);
    frame_ = document->GetFrame();
    if (frame_)
      frame_->FrameScheduler()->SetPaused(true);
  }

  ~LoopSuspensionScope() {
    if (frame_)
      frame_->FrameScheduler()->SetPaused(false);
  }

 private:
  v8::Isolate::SuppressMicrotaskExecutionScope suppress_microtasks_;
  Persistent<LocalFrame> frame_;
};

}  // namespace

PreemptionManager::PreemptionManager(v8::Isolate* isolate)
    : isolate_(isolate), weak_ptr_factory_(this) {}
PreemptionManager::~PreemptionManager() {}

bool PreemptionManager::InPreemptionCheckpointScope() const {
  return in_preemption_checkpoint_;
}

void PreemptionManager::EnterPreemptionCheckpointScope() {
  // TRACE_EVENT_ASYNC_BEGIN0("tzik", "PreemptionCheckpointScope", this);

  DCHECK(!in_preemption_checkpoint_);
  in_preemption_checkpoint_ = true;

  if (in_preemption_opt_in_ && WTF::TimeTicks::Now() > next_)
    RunNestedLoop();
}

void PreemptionManager::LeavePreemptionCheckpointScope() {
  DCHECK(in_preemption_checkpoint_);
  in_preemption_checkpoint_ = false;

  // TRACE_EVENT_ASYNC_END0("tzik", "PreemptionCheckpointScope", this);
}

bool PreemptionManager::InPreemptionOptInScope() const {
  return in_preemption_opt_in_;
}

void PreemptionManager::EnterPreemptionOptInScope() {
  // TRACE_EVENT_ASYNC_BEGIN0("tzik", "PreemptionOptInScope", this);

  DCHECK(!in_preemption_opt_in_);
  in_preemption_opt_in_ = true;
}

void PreemptionManager::LeavePreemptionOptInScope() {
  DCHECK(in_preemption_opt_in_);
  in_preemption_opt_in_ = false;

  // TRACE_EVENT_ASYNC_END0("tzik", "PreemptionOptInScope", this);
}

WTF::WeakPtr<PreemptionManager> PreemptionManager::GetWeakPtr() {
  return weak_ptr_factory_.CreateWeakPtr();
}

void PreemptionManager::RunNestedLoop() {
  LoopSuspensionScope scope(isolate_);

  base::RunLoop run_loop(base::RunLoop::Type::kNestableTasksAllowed);
  base::ThreadTaskRunnerHandle::Get()->PostTask(FROM_HERE,
                                                run_loop.QuitClosure());
  run_loop.Run();
  next_ = WTF::TimeTicks::Now() + kNestedLoopMinimumInterval;
}

}  // namespace blink
