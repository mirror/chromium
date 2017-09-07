// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/webaudio/AudioWorkletMessagingProxy.h"

#include "core/dom/TaskRunnerHelper.h"
#include "modules/webaudio/AudioWorkletGlobalScope.h"
#include "modules/webaudio/AudioWorkletNode.h"
#include "modules/webaudio/AudioWorkletObjectProxy.h"
#include "modules/webaudio/AudioWorkletProcessor.h"
#include "modules/webaudio/AudioWorkletThread.h"
#include "modules/webaudio/CrossThreadAudioWorkletProcessorInfo.h"

namespace blink {

AudioWorkletMessagingProxy::AudioWorkletMessagingProxy(
    ExecutionContext* execution_context,
    WorkerClients* worker_clients)
    : ThreadedWorkletMessagingProxy(execution_context, worker_clients) {}

AudioWorkletMessagingProxy::~AudioWorkletMessagingProxy() {}

void AudioWorkletMessagingProxy::CreateProcessor(
    AudioWorkletHandler* handler) {
  DCHECK(IsMainThread());
  TaskRunnerHelper::Get(TaskType::kMiscPlatformAPI, GetWorkerThread())
      ->PostTask(
          BLINK_FROM_HERE,
          CrossThreadBind(
              &AudioWorkletMessagingProxy::CreateProcessorOnRenderingThread,
              WrapCrossThreadPersistent(this),
              CrossThreadUnretained(handler),
              handler->Name(),
              CrossThreadUnretained(GetWorkerThread())));
}

void AudioWorkletMessagingProxy::CreateProcessorOnRenderingThread(
    AudioWorkletHandler* handler, const String& name, WorkerThread* worker_thread) {
  AudioWorkletGlobalScope* global_scope =
      static_cast<AudioWorkletGlobalScope*>(worker_thread->GlobalScope());
  AudioWorkletProcessor* processor = global_scope->CreateInstance(name);
  handler->SetProcessorOnRenderingThread(processor);
}

void AudioWorkletMessagingProxy::SynchronizeWorkletProcessorInfoList(
    std::unique_ptr<Vector<CrossThreadAudioWorkletProcessorInfo>> info_list) {
  DCHECK(IsMainThread());
  for (auto& processor_info : *info_list) {
    processor_info_map_.insert(processor_info.Name(),
                               processor_info.ParamInfoList());
  }
}

bool AudioWorkletMessagingProxy::IsProcessorRegistered(
    const String& name) const {
  return processor_info_map_.Contains(name);
}

const Vector<CrossThreadAudioParamInfo>
AudioWorkletMessagingProxy::GetParamInfoListForProcessor(
    const String& name) const {
  DCHECK(IsProcessorRegistered(name));
  return processor_info_map_.at(name);
}

std::unique_ptr<ThreadedWorkletObjectProxy>
AudioWorkletMessagingProxy::CreateObjectProxy(
    ThreadedWorkletMessagingProxy* messaging_proxy,
    ParentFrameTaskRunners* parent_frame_task_runners) {
  return WTF::MakeUnique<AudioWorkletObjectProxy>(
      static_cast<AudioWorkletMessagingProxy*>(messaging_proxy),
      parent_frame_task_runners);
}

std::unique_ptr<WorkerThread> AudioWorkletMessagingProxy::CreateWorkerThread() {
  return AudioWorkletThread::Create(CreateThreadableLoadingContext(),
                                    WorkletObjectProxy());
}

}  // namespace blink
