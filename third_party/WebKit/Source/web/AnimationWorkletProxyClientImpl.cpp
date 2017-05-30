// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "web/AnimationWorkletProxyClientImpl.h"

#include "core/dom/CompositorProxy.h"
#include "platform/graphics/CompositorMutableStateProvider.h"
#include "web/CompositorMutatorImpl.h"

namespace blink {

AnimationWorkletProxyClientImpl::AnimationWorkletProxyClientImpl(
    CompositorMutatorImpl* mutator)
    : mutator_(mutator) {
  DCHECK(IsMainThread());
}

DEFINE_TRACE(AnimationWorkletProxyClientImpl) {
  AnimationWorkletProxyClient::Trace(visitor);
  CompositorAnimator::Trace(visitor);
}

void AnimationWorkletProxyClientImpl::SetGlobalScope(
    WorkletGlobalScope* scope) {
  DCHECK(!IsMainThread());
  DCHECK(scope);
  global_scope_ = static_cast<AnimationWorkletGlobalScope*>(scope);
  mutator_->RegisterCompositorAnimator(this);
}

void AnimationWorkletProxyClientImpl::Dispose() {
  // At worklet scope termination break the reference cycle between
  // CompositorMutatorImpl and AnimationProxyClientImpl and also the cycle
  // between AnimationWorkletGlobalScope and AnimationWorkletProxyClientImpl.
  mutator_->UnregisterCompositorAnimator(this);
  global_scope_ = nullptr;
}

bool AnimationWorkletProxyClientImpl::Mutate(
    double monotonic_time_now,
    CompositorMutableStateProvider* provider) {
  DCHECK(!IsMainThread());

  if (global_scope_)
    global_scope_->Mutate();

  // Always request another rAF for now.
  return true;
}

}  // namespace blink
