// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/compositorworker/AnimationWorkletThread.h"

#include "core/workers/GlobalScopeCreationParams.h"
#include "core/workers/WorkerThread.h"
#include "core/workers/WorkletThreadHolder.h"
#include "modules/compositorworker/AnimationWorkletGlobalScope.h"
#include "platform/instrumentation/tracing/TraceEvent.h"
#include "platform/weborigin/SecurityOrigin.h"
#include "platform/wtf/PtrUtil.h"

namespace blink {

template class WorkletThreadHolder<AnimationWorkletThread>;

std::unique_ptr<AnimationWorkletThread> AnimationWorkletThread::Create(
    ThreadableLoadingContext* loading_context,
    WorkerReportingProxy& worker_reporting_proxy) {
  TRACE_EVENT0(TRACE_DISABLED_BY_DEFAULT("animation-worklet"),
               "AnimationWorkletThread::create");
  DCHECK(IsMainThread());
  return WTF::WrapUnique(
      new AnimationWorkletThread(loading_context, worker_reporting_proxy));
}

AnimationWorkletThread::AnimationWorkletThread(
    ThreadableLoadingContext* loading_context,
    WorkerReportingProxy& worker_reporting_proxy)
    : AbstractAnimationWorkletThread(loading_context, worker_reporting_proxy) {}

AnimationWorkletThread::~AnimationWorkletThread() {}

WorkerBackingThread& AnimationWorkletThread::GetWorkerBackingThread() {
  return *WorkletThreadHolder<AnimationWorkletThread>::GetInstance()
              ->GetThread();
}

void CollectAllGarbageOnAnimationWorkletThread(WaitableEvent* done_event) {
  blink::ThreadState::Current()->CollectAllGarbage();
  done_event->Signal();
}

void AnimationWorkletThread::CollectAllGarbage() {
  DCHECK(IsMainThread());
  WaitableEvent done_event;
  WorkletThreadHolder<AnimationWorkletThread>* worklet_thread_holder =
      WorkletThreadHolder<AnimationWorkletThread>::GetInstance();
  if (!worklet_thread_holder)
    return;
  worklet_thread_holder->GetThread()->BackingThread().PostTask(
      BLINK_FROM_HERE,
      CrossThreadBind(&CollectAllGarbageOnAnimationWorkletThread,
                      CrossThreadUnretained(&done_event)));
  done_event.Wait();
}

void AnimationWorkletThread::EnsureSharedBackingThread() {
  DCHECK(IsMainThread());
  WorkletThreadHolder<AnimationWorkletThread>::EnsureInstance(
      "AnimationWorkletThread");
}

void AnimationWorkletThread::ClearSharedBackingThread() {
  DCHECK(IsMainThread());
  WorkletThreadHolder<AnimationWorkletThread>::ClearInstance();
}

WebThread* AnimationWorkletThread::GetSharedBackingThread() {
  DCHECK(IsMainThread());
  WorkletThreadHolder<AnimationWorkletThread>* instance =
      WorkletThreadHolder<AnimationWorkletThread>::GetInstance();
  if (!instance)
    return nullptr;
  return &(instance->GetThread()->BackingThread().PlatformThread());
}

void AnimationWorkletThread::CreateSharedBackingThreadForTest() {
  WorkletThreadHolder<AnimationWorkletThread>::CreateForTest(
      "AnimationWorkletThread");
}

WorkerOrWorkletGlobalScope* AnimationWorkletThread::CreateWorkerGlobalScope(
    std::unique_ptr<GlobalScopeCreationParams> creation_params) {
  TRACE_EVENT0(TRACE_DISABLED_BY_DEFAULT("animation-worklet"),
               "AnimationWorkletThread::createWorkerGlobalScope");

  RefPtr<SecurityOrigin> security_origin =
      SecurityOrigin::Create(creation_params->script_url);
  if (creation_params->starter_origin_privilege_data) {
    security_origin->TransferPrivilegesFrom(
        std::move(creation_params->starter_origin_privilege_data));
  }

  return AnimationWorkletGlobalScope::Create(
      creation_params->script_url, creation_params->user_agent,
      std::move(security_origin), this->GetIsolate(), this,
      creation_params->worker_clients);
}

}  // namespace blink
