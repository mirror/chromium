// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/animationworklet/AnimationWorkletProxyClientImpl.h"

#include "core/dom/Document.h"
#include "core/frame/LocalFrame.h"
#include "core/frame/WebLocalFrameImpl.h"
#include "core/workers/WorkerThread.h"
#include "platform/WaitableEvent.h"
#include "platform/graphics/CompositorMutatorImpl.h"

namespace blink {

AnimationWorkletProxyClientImpl::AnimationWorkletProxyClientImpl(
    CompositorMutatorImpl* mutator)
    : mutator_(mutator) {
  DCHECK(IsMainThread());
}

void AnimationWorkletProxyClientImpl::Trace(blink::Visitor* visitor) {
  AnimationWorkletProxyClient::Trace(visitor);
  CompositorAnimator::Trace(visitor);
}

void AnimationWorkletProxyClientImpl::SetGlobalScope(
    WorkletGlobalScope* global_scope) {
  DCHECK(global_scope);
  DCHECK(global_scope->IsContextThread());
  global_scope_ = static_cast<AnimationWorkletGlobalScope*>(global_scope);
  task_runner_ =
      global_scope_->GetThread()->GetTaskRunner(TaskType::kMiscPlatformAPI);
  if (mutator_)
    mutator_->RegisterCompositorAnimatorOnWorkletThread(this);
}

// The rest of the calls should handle the case where there is no scope.
// It may be delayed or never coming.

void AnimationWorkletProxyClientImpl::Dispose() {
  if (!global_scope_)
    return;

  DCHECK(global_scope_->IsContextThread());

  // At worklet scope termination break the reference cycle between
  // AnimationWorkletGlobalScope and AnimationWorkletProxyClientImpl.
  global_scope_ = nullptr;

  // Further, we will not notify the mutator of anything.
  mutator_ = nullptr;
}

void AnimationWorkletProxyClientImpl::MutateWithEvent(
    const CompositorMutatorInputState* input_state,
    std::unique_ptr<MustSignal> is_done) {
  if (!global_scope_)
    return;

  DCHECK(global_scope_->IsContextThread());
  if (!mutator_)
    return;

  mutator_->SetMutationUpdateOnWorkletThread(
      global_scope_->Mutate(*input_state));

  // and is_done Signal()s on destruction
}

void AnimationWorkletProxyClientImpl::SealWithEvent(
    std::unique_ptr<MustSignal> is_done) {
  if (global_scope_) {
    DCHECK(global_scope_->IsContextThread());
  }

  // we will not notify the mutator of anything.
  mutator_ = nullptr;

  // and is_done Signal()s on destruction
}

void AnimationWorkletProxyClientImpl::Mutate(
    const CompositorMutatorInputState& state) {
  // Nothing should call mutate before we get a SetGlobalScope and then
  // reach out to register ourselves.
  DCHECK(task_runner_);
  // You can't block waiting for yourself
  DCHECK(!task_runner_->BelongsToCurrentThread());

  // TODO(petermayo) Schedule a pending mutation rather than block on
  // the mutation to complete. (Or ensure a mutation is pending).
  // crbug.com/767210
  WaitableEvent is_done;
  PostCrossThreadTask(
      *task_runner_, FROM_HERE,
      CrossThreadBind(&AnimationWorkletProxyClientImpl::MutateWithEvent,
                      WrapCrossThreadPersistent(this),
                      CrossThreadUnretained(&state),
                      WTF::Passed(std::make_unique<MustSignal>(&is_done))));
  is_done.Wait();
}

void AnimationWorkletProxyClientImpl::Seal() {
  // You can't block waiting for yourself
  DCHECK(!task_runner_->BelongsToCurrentThread());

  WaitableEvent is_done;
  PostCrossThreadTask(
      *task_runner_, FROM_HERE,
      CrossThreadBind(&AnimationWorkletProxyClientImpl::SealWithEvent,
                      WrapCrossThreadPersistent(this),
                      WTF::Passed(std::make_unique<MustSignal>(&is_done))));
  is_done.Wait();
}

// static
AnimationWorkletProxyClientImpl* AnimationWorkletProxyClientImpl::FromDocument(
    Document* document) {
  WebLocalFrameImpl* local_frame =
      WebLocalFrameImpl::FromFrame(document->GetFrame());
  return new AnimationWorkletProxyClientImpl(
      local_frame->LocalRootFrameWidget()->CompositorMutator());
}

AnimationWorkletProxyClientImpl::MustSignal::MustSignal(WaitableEvent* event)
    : event_(event) {}

AnimationWorkletProxyClientImpl::MustSignal::~MustSignal() {
  if (event_)
    Signal();
}

void AnimationWorkletProxyClientImpl::MustSignal::Signal() {
  if (event_)
    event_->Signal();
  event_ = nullptr;
}

}  // namespace blink
