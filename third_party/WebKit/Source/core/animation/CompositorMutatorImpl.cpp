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
    : client_(nullptr), weak_factory_(this) {}
CompositorMutatorImpl::~CompositorMutatorImpl() {}

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
  DCHECK(Platform::Current()->CompositorThread()->IsCurrentThread());
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
  DCHECK(!Platform::Current()->CompositorThread()->IsCurrentThread());
  Platform::Current()->CompositorThread()->GetWebTaskRunner()->PostTask(
      BLINK_FROM_HERE,
      CrossThreadBind(
          &CompositorMutatorImpl::RegisterCompositorAnimatorOnCompositorThread,
          GetWeakPtr(), CrossThreadPersistent<CompositorAnimator>(animator)));
}

void CompositorMutatorImpl::UnregisterCompositorAnimatorOnWorkletThread(
    CompositorAnimator* animator) {
  DCHECK(animator);
  DCHECK(!Platform::Current()->CompositorThread()->IsCurrentThread());
  Platform::Current()->CompositorThread()->GetWebTaskRunner()->PostTask(
      BLINK_FROM_HERE,
      CrossThreadBind(&CompositorMutatorImpl::
                          UnregisterCompositorAnimatorOnCompositorThread,
                      GetWeakPtr(),
                      CrossThreadPersistent<CompositorAnimator>(animator)));
}

void CompositorMutatorImpl::RegisterCompositorAnimatorOnCompositorThread(
    CompositorAnimator* animator) {
  TRACE_EVENT0(
      "cc",
      "CompositorMutatorImpl::RegisterCompositorAnimatorOnCompositorThread");
  DCHECK(Platform::Current()->CompositorThread()->IsCurrentThread());
  DCHECK(!animators_.Contains(animator));
  animators_.insert(animator);
}

void CompositorMutatorImpl::UnregisterCompositorAnimatorOnCompositorThread(
    CompositorAnimator* animator) {
  TRACE_EVENT0(
      "cc",
      "CompositorMutatorImpl::UnregisterCompositorAnimatorOnCompositorThread");
  DCHECK(Platform::Current()->CompositorThread()->IsCurrentThread());
  DCHECK(animators_.Contains(animator));
  animators_.erase(animator);
}

void CompositorMutatorImpl::SetMutationUpdateOnWorkletThread(
    std::unique_ptr<CompositorMutatorOutputState> state) {
  DCHECK(!Platform::Current()->CompositorThread()->IsCurrentThread());
  DCHECK(state);

  // This is called during animator->Mutate(*state); above.
  outputs_.push_back(std::move(state));
}
}  // namespace blink
