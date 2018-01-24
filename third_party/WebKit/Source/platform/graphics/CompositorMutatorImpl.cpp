// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/graphics/CompositorMutatorImpl.h"

#include "platform/CrossThreadFunctional.h"
#include "platform/WaitableEvent.h"
#include "platform/WebTaskRunner.h"
#include "platform/graphics/CompositorAnimator.h"
#include "platform/graphics/CompositorMutatorClient.h"
#include "platform/heap/Handle.h"
#include "platform/instrumentation/tracing/TraceEvent.h"
#include "platform/wtf/PtrUtil.h"

namespace blink {

CompositorMutatorImpl::CompositorMutatorImpl()
    : client_(nullptr), weak_factory_(this) {
  if (Platform::Current()->CompositorThread())
    mutator_thread_ = Platform::Current()->CompositorThread();
  else
    mutator_thread_ = Platform::Current()->MainThread();
}

CompositorMutatorImpl::~CompositorMutatorImpl() {
  for (CompositorAnimator* animator : animators_) {
    animator->Seal();
  }
  animators_.clear();
}

std::unique_ptr<CompositorMutatorClient> CompositorMutatorImpl::CreateClient() {
  DCHECK(IsMainThread());
  CompositorMutatorImpl* mutator = new CompositorMutatorImpl();
  std::unique_ptr<CompositorMutatorClient> mutator_client =
      std::make_unique<CompositorMutatorClient>(mutator);
  mutator->SetClient(mutator_client.get());
  return mutator_client;
}

void CompositorMutatorImpl::Mutate(
    std::unique_ptr<CompositorMutatorInputState> state) {
  TRACE_EVENT0("cc", "CompositorMutatorImpl::mutate");
  DCHECK(mutator_thread_->IsCurrentThread());
  DCHECK(client_);
  for (CompositorAnimator* animator : animators_) {
    animator->Mutate(*state);
  }
  while (!outputs_.empty()) {
    client_->SetMutationUpdate(std::move(outputs_.back()));
    outputs_.pop_back();
  }
}

void CompositorMutatorImpl::RegisterCompositorAnimatorOnWorkletThread(
    CompositorAnimator* animator) {
  DCHECK(animator);
  DCHECK(!mutator_thread_->IsCurrentThread());
  PostCrossThreadTask(
      *mutator_thread_->GetWebTaskRunner(), FROM_HERE,
      CrossThreadBind(
          &CompositorMutatorImpl::RegisterCompositorAnimatorOnMutatorThread,
          GetWeakPtr(), CrossThreadPersistent<CompositorAnimator>(animator)));
}

void CompositorMutatorImpl::UnregisterCompositorAnimatorOnWorkletThread(
    CompositorAnimator* animator) {
  DCHECK(animator);
  DCHECK(!mutator_thread_->IsCurrentThread());
  PostCrossThreadTask(
      *mutator_thread_->GetWebTaskRunner(), FROM_HERE,
      CrossThreadBind(
          &CompositorMutatorImpl::UnregisterCompositorAnimatorOnMutatorThread,
          GetWeakPtr(), CrossThreadPersistent<CompositorAnimator>(animator)));
}

void CompositorMutatorImpl::RegisterCompositorAnimatorOnMutatorThread(
    CompositorAnimator* animator) {
  TRACE_EVENT0(
      "cc", "CompositorMutatorImpl::RegisterCompositorAnimatorOnMutatorThread");
  DCHECK(mutator_thread_->IsCurrentThread());
  DCHECK(!animators_.Contains(animator));
  animators_.insert(animator);
}

void CompositorMutatorImpl::UnregisterCompositorAnimatorOnMutatorThread(
    CompositorAnimator* animator) {
  TRACE_EVENT0(
      "cc",
      "CompositorMutatorImpl::UnregisterCompositorAnimatorOnMutatorThread");
  DCHECK(mutator_thread_->IsCurrentThread());
  DCHECK(animators_.Contains(animator));
  animators_.erase(animator);
}

void CompositorMutatorImpl::SetMutationUpdateOnWorkletThread(
    std::unique_ptr<CompositorMutatorOutputState> state) {
  DCHECK(!mutator_thread_->IsCurrentThread());
  DCHECK(state);

  // This is called during animator->Mutate(*state); above.
  outputs_.push_back(std::move(state));
}
}  // namespace blink
