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

namespace {

void CreateCompositorMutatorClient(
    std::unique_ptr<CompositorMutatorClient>* ptr,
    WaitableEvent* done_event) {
  CompositorMutatorImpl* mutator = CompositorMutatorImpl::Create();
  ptr->reset(new CompositorMutatorClient(mutator));
  mutator->SetClient(ptr->get());
  done_event->Signal();
}

}  // namespace

CompositorMutatorImpl::CompositorMutatorImpl()
    : has_animators_(false),
      client_(nullptr),
      thread_(Platform::Current()->CurrentThread()) {}

std::unique_ptr<CompositorMutatorClient> CompositorMutatorImpl::CreateClient() {
  std::unique_ptr<CompositorMutatorClient> mutator_client;
  WaitableEvent done_event;
  if (WebThread* compositor_thread = Platform::Current()->CompositorThread()) {
    compositor_thread->GetWebTaskRunner()->PostTask(
        BLINK_FROM_HERE, CrossThreadBind(&CreateCompositorMutatorClient,
                                         CrossThreadUnretained(&mutator_client),
                                         CrossThreadUnretained(&done_event)));
  } else {
    CreateCompositorMutatorClient(&mutator_client, &done_event);
  }
  // TODO(flackr): Instead of waiting for this event, we may be able to just set
  // the mutator on the CompositorWorkerProxyClient directly from the compositor
  // thread before it gets used there. We still need to make sure we only
  // create one mutator though.
  done_event.Wait();
  return mutator_client;
}

CompositorMutatorImpl* CompositorMutatorImpl::Create() {
  return new CompositorMutatorImpl();
}

void CompositorMutatorImpl::Mutate(
    std::unique_ptr<CompositorMutatorInputState> state) {
  TRACE_EVENT0("cc", "CompositorMutatorImpl::mutate");
  DCHECK(thread_->IsCurrentThread());
  if (has_animators_) {
    for (CompositorAnimator* animator : animators_) {
      animator->Mutate(*state);
    }
  }
}

bool CompositorMutatorImpl::HasAnimators() {
  return has_animators_;
}

void CompositorMutatorImpl::RegisterCompositorAnimator(
    CompositorAnimator* animator) {
  DCHECK(thread_);
  has_animators_ = true;
  thread_->GetWebTaskRunner()->PostTask(
      BLINK_FROM_HERE,
      CrossThreadBind(
          &CompositorMutatorImpl::RegisterCompositorAnimatorInternal,
          CrossThreadUnretained(this),
          CrossThreadPersistent<CompositorAnimator>(animator)));
}

void CompositorMutatorImpl::UnregisterCompositorAnimator(
    CompositorAnimator* animator) {
  DCHECK(animator);
  DCHECK(thread_);
  thread_->GetWebTaskRunner()->PostTask(
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
  DCHECK(thread_);
  DCHECK(thread_->IsCurrentThread());
  DCHECK(!animators_.Contains(animator));
  animators_.insert(animator);
  has_animators_ = !animators_.IsEmpty();
}

void CompositorMutatorImpl::UnregisterCompositorAnimatorInternal(
    CompositorAnimator* animator) {
  TRACE_EVENT0("cc",
               "CompositorMutatorImpl::UnregisterCompositorAnimatorInternal");
  DCHECK(thread_);
  DCHECK(thread_->IsCurrentThread());
  DCHECK(animators_.Contains(animator));
  animators_.erase(animator);
  has_animators_ = !animators_.IsEmpty();
}

void CompositorMutatorImpl::SetMutationUpdate(
    std::unique_ptr<CompositorMutatorOutputState> state) {
  DCHECK(thread_);
  thread_->GetWebTaskRunner()->PostTask(
      BLINK_FROM_HERE,
      CrossThreadBind(&CompositorMutatorClient::SetMutationUpdate,
                      CrossThreadUnretained(client_),
                      WTF::Passed(std::move(state))));
}

}  // namespace blink
