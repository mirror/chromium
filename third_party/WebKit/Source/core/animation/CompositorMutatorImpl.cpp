// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/animation/CompositorMutatorImpl.h"

#include "core/animation/CompositorAnimator.h"
#include "platform/CrossThreadFunctional.h"
#include "platform/WaitableEvent.h"
#include "platform/WebTaskRunner.h"
#include "platform/graphics/CompositorMutatorClient.h"
#include "platform/heap/Handle.h"
#include "platform/instrumentation/tracing/TraceEvent.h"
#include "platform/wtf/PtrUtil.h"
#include "public/platform/Platform.h"

namespace blink {

CompositorMutatorImpl::CompositorMutatorImpl()
    : has_animators_(false), client_(nullptr) {}

std::unique_ptr<CompositorMutatorImpl> CompositorMutatorImpl::Create() {
  std::unique_ptr<CompositorMutatorImpl> mutator(new CompositorMutatorImpl());
  return mutator;
}

std::unique_ptr<CompositorMutatorClient> CompositorMutatorImpl::CreateClient() {
  std::unique_ptr<CompositorMutatorClient> mutator_client;
  mutator_client.reset(new CompositorMutatorClient(this));
  SetClient(mutator_client.get());
  return mutator_client;
}

void CompositorMutatorImpl::SetClient(CompositorMutatorClient* client) {
  client_ = client;
}

void CompositorMutatorImpl::Mutate(
    std::unique_ptr<CompositorMutatorInputState> state) {
  TRACE_EVENT0("cc", "CompositorMutatorImpl::mutate");
  DCHECK(Platform::Current()->CompositorThread()->IsCurrentThread());
  if (has_animators_) {
    for (CompositorAnimator* animator : animators_) {
      animator->Mutate(*state);
    }
  }
  while (!outputs_.empty()) {
    if (client_ && outputs_.back())
      client_->SetMutationUpdate(std::move(outputs_.back()));
    outputs_.pop_back();
  }
}

void CompositorMutatorImpl::RegisterCompositorAnimator(
    CompositorAnimator* animator) {
  has_animators_ = true;
  Platform::Current()->CompositorThread()->GetWebTaskRunner()->PostTask(
      BLINK_FROM_HERE,
      CrossThreadBind(
          &CompositorMutatorImpl::RegisterCompositorAnimatorInternal,
          CrossThreadUnretained(this),
          CrossThreadPersistent<CompositorAnimator>(animator)));
}

void CompositorMutatorImpl::UnregisterCompositorAnimator(
    CompositorAnimator* animator) {
  DCHECK(animator);
  Platform::Current()->CompositorThread()->GetWebTaskRunner()->PostTask(
      BLINK_FROM_HERE,
      CrossThreadBind(
          &CompositorMutatorImpl::UnregisterCompositorAnimatorInternal,
          CrossThreadUnretained(this),
          CrossThreadPersistent<CompositorAnimator>(animator)));
}

void CompositorMutatorImpl::RegisterCompositorAnimatorInternal(
    CompositorAnimator* animator) {
  TRACE_EVENT0("cc",
               "CompositorMutatorImpl::RegisterCompositorAnimatorInternal");
  DCHECK(!animators_.Contains(animator));
  animators_.insert(animator);
  has_animators_ = !animators_.IsEmpty();
}

void CompositorMutatorImpl::UnregisterCompositorAnimatorInternal(
    CompositorAnimator* animator) {
  TRACE_EVENT0("cc",
               "CompositorMutatorImpl::UnregisterCompositorAnimatorInternal");
  DCHECK(animators_.Contains(animator));
  animators_.erase(animator);
  has_animators_ = !animators_.IsEmpty();
}

void CompositorMutatorImpl::SetMutationUpdate(
    std::unique_ptr<CompositorMutatorOutputState> state) {
  DCHECK(!Platform::Current()->CompositorThread()->IsCurrentThread());
  DCHECK(state);

  // This is called during animator->Mutate(*state); above.
  outputs_.push_back(std::move(state));
}

}  // namespace blink
