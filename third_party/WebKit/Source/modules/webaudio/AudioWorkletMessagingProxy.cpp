// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/webaudio/AudioWorkletMessagingProxy.h"

#include "core/dom/TaskRunnerHelper.h"
#include "core/workers/ThreadedWorkletObjectProxy.h"
#include "modules/webaudio/AudioWorkletGlobalScope.h"
#include "modules/webaudio/AudioWorkletNode.h"
#include "modules/webaudio/AudioWorkletThread.h"

namespace blink {

AudioWorkletMessagingProxy::AudioWorkletMessagingProxy(
    ExecutionContext* execution_context,
    WorkerClients* worker_clients)
    : ThreadedWorkletMessagingProxy(execution_context, worker_clients) {}

void AudioWorkletMessagingProxy::CreateProcessor(
    AudioWorkletHandler* handler) {
  DCHECK(IsMainThread());
  TaskRunnerHelper::Get(TaskType::kMiscPlatformAPI, GetWorkerThread())
      ->PostTask(
          BLINK_FROM_HERE,
          CrossThreadBind(
              &AudioWorkletMessagingProxy::CreateProcessorOnWorkerThread,
              WrapCrossThreadPersistent(this),
              CrossThreadUnretained(handler),
              CrossThreadUnretained(GetWorkerThread())));
}

void AudioWorkletMessagingProxy::CreateProcessorOnWorkerThread(
    AudioWorkletHandler* handler, WorkerThread* worker_thread) {
  AudioWorkletGlobalScope* global_scope =
      static_cast<AudioWorkletGlobalScope*>(worker_thread->GlobalScope());
  global_scope->CreateInstance(handler->name());
}

AudioWorkletMessagingProxy::~AudioWorkletMessagingProxy() {}

std::unique_ptr<WorkerThread> AudioWorkletMessagingProxy::CreateWorkerThread() {
  return AudioWorkletThread::Create(CreateThreadableLoadingContext(),
                                    WorkletObjectProxy());
}

}  // namespace blink
