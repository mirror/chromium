// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/compositorworker/AnimationWorkletProxyClientImpl.h"

#include "core/animation/CompositorMutatorImpl.h"
#include "core/dom/Document.h"
#include "core/frame/LocalFrame.h"
#include "core/frame/WebLocalFrameImpl.h"
#include "modules/compositorworker/AnimationWorkletThread.h"
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
  mutator_->RegisterCompositorAnimator(this);
}

void AnimationWorkletProxyClientImpl::Dispose() {
  DCHECK(global_scope_->IsContextThread());
  // At worklet scope termination break the reference cycle between
  // CompositorMutatorImpl and AnimationProxyClientImpl and also the cycle
  // between AnimationWorkletGlobalScope and AnimationWorkletProxyClientImpl.
  mutator_->UnregisterCompositorAnimator(this);
  global_scope_ = nullptr;
}

void AnimationWorkletProxyClientImpl::Mutate(
    const CompositorMutatorInputState& state) {
  DCHECK(!IsMainThread() && !global_scope_->IsContextThread());

  std::unique_ptr<CompositorMutatorOutputState> output = nullptr;

  if (global_scope_) {
    // TODO(petermayo) Schedule a pending mutation rather than block on
    // the mutation to complete. (Or ensure a mutation is pending).
    // crbug.com/767210
    WaitableEvent is_done;
    AnimationWorkletThread::GetSharedBackingThread()
        ->GetSingleThreadTaskRunner()
        ->PostTask(
            BLINK_FROM_HERE,
            ConvertToBaseCallback(WTF::Bind(
                &AnimationWorkletGlobalScope::MutateWithEvent, global_scope_,
                CrossThreadUnretained(&state), CrossThreadUnretained(&output),
                CrossThreadUnretained(&is_done))));
    is_done.Wait();
  }
  mutator_->SetMutationUpdate(std::move(output));
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
