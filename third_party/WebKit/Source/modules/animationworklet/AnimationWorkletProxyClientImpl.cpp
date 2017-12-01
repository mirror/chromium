// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/animationworklet/AnimationWorkletProxyClientImpl.h"

#include "core/animation/CompositorMutatorImpl.h"
#include "core/dom/Document.h"
#include "core/frame/LocalFrame.h"
#include "core/frame/WebLocalFrameImpl.h"
#include "modules/animationworklet/AnimationWorkletThread.h"
#include "platform/WaitableEvent.h"

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
  mutator_->RegisterCompositorAnimator(this);
}

void AnimationWorkletProxyClientImpl::Dispose() {
  CrossThreadPersistent<AnimationWorkletGlobalScope> global_scope =
      global_scope_.Get();
  if (!global_scope)
    return;

  DCHECK(global_scope->IsContextThread());
  // At worklet scope termination break the reference cycle between
  // CompositorMutatorImpl and AnimationProxyClientImpl.
  global_scope_ = nullptr;
  mutator_->UnregisterCompositorAnimator(this);
}

void AnimationWorkletProxyClientImpl::MutateWithEvent(
    const CompositorMutatorInputState* input_state,
    WaitableEvent* is_done) {
  CrossThreadPersistent<AnimationWorkletGlobalScope> global_scope =
      global_scope_.Get();
  if (!global_scope)
    return;

  if (global_scope) {
    DCHECK(global_scope->IsContextThread());
    mutator_->SetMutationUpdate(global_scope->Mutate(*input_state));
  }
  is_done->Signal();
}

void AnimationWorkletProxyClientImpl::Mutate(
    const CompositorMutatorInputState& state) {
  DCHECK(Platform::Current()->CompositorThread()->IsCurrentThread());

  CrossThreadPersistent<AnimationWorkletGlobalScope> global_scope =
      global_scope_.Get();
  if (!global_scope)
    return;

  // TODO(petermayo) Schedule a pending mutation rather than block on
  // the mutation to complete. (Or ensure a mutation is pending).
  // crbug.com/767210
  WaitableEvent is_done;
  task_runner_->PostTask(
      BLINK_FROM_HERE,
      CrossThreadBind(&AnimationWorkletProxyClientImpl::MutateWithEvent,
                      WrapCrossThreadPersistent(this),
                      CrossThreadUnretained(&state),
                      CrossThreadUnretained(&is_done)));
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

}  // namespace blink
