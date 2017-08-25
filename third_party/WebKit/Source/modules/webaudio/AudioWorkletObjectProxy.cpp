// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/webaudio/AudioWorkletObjectProxy.h"

#include "bindings/core/v8/ScriptSourceCode.h"
#include "bindings/core/v8/WorkerOrWorkletScriptController.h"
#include "core/workers/ThreadedWorkletMessagingProxy.h"
#include "core/workers/WorkerThread.h"
#include "modules/webaudio/CrossThreadAudioWorkletProcessorInfo.h"
#include "modules/webaudio/AudioWorkletGlobalScope.h"
#include "modules/webaudio/AudioWorkletMessagingProxy.h"
#include "platform/CrossThreadFunctional.h"

namespace blink {

AudioWorkletObjectProxy::AudioWorkletObjectProxy(
    AudioWorkletMessagingProxy* messaging_proxy_weak_ptr,
    ParentFrameTaskRunners* parent_frame_task_runners)
    : ThreadedWorkletObjectProxy(
          static_cast<ThreadedWorkletMessagingProxy*>(messaging_proxy_weak_ptr),
          parent_frame_task_runners) {}

void AudioWorkletObjectProxy::EvaluateScript(const String& source,
                                             const KURL& script_url,
                                             WorkerThread* worker_thread) {
  AudioWorkletGlobalScope* global_scope =
      ToAudioWorkletGlobalScope(worker_thread->GlobalScope());
  global_scope->ScriptController()->Evaluate(
      ScriptSourceCode(source, script_url));

  if (global_scope->NumberOfRegisteredDefinitions() == 0)
    return;

  std::unique_ptr<Vector<CrossThreadAudioWorkletProcessorInfo>>
      processor_info_list =
          global_scope->WorkletProcessorInfoListForSynchronization();

  if (processor_info_list->size() == 0)
    return;

  GetParentFrameTaskRunners()->Get(TaskType::kUnthrottled)->PostTask(
      BLINK_FROM_HERE,
      CrossThreadBind(
          &AudioWorkletMessagingProxy::SynchronizeWorkletProcessorInfoList,
          GetAudioWorkletMessagingProxyWeakPtr(),
          WTF::Passed(std::move(processor_info_list))));
}

CrossThreadWeakPersistent<AudioWorkletMessagingProxy>
AudioWorkletObjectProxy::GetAudioWorkletMessagingProxyWeakPtr() {
  return static_cast<AudioWorkletMessagingProxy*>(
      MessagingProxyWeakPtr().Get());
}

}  // namespace blink
