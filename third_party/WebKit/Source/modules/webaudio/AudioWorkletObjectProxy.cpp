// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/webaudio/AudioWorkletObjectProxy.h"

#include "bindings/core/v8/ScriptSourceCode.h"
#include "bindings/core/v8/WorkerOrWorkletScriptController.h"
#include "core/workers/ThreadedWorkletMessagingProxy.h"
#include "core/workers/WorkerThread.h"
#include "modules/webaudio/AudioWorkletGlobalScope.h"
#include "modules/webaudio/AudioWorkletMessagingProxy.h"
#include "modules/webaudio/AudioWorkletProcessorInfo.h"

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

  if (global_scope->GetNumberOfRegisteredDefinitions() == 0)
    return;

  std::unique_ptr<Vector<AudioWorkletProcessorInfo>> info_list =
      global_scope->GenerateProcessorInfoForSynchronization();

  // TODO(crbug.com/755566): Extract/build the information for synchronization
  // and send it to the associated AudioWorkletMessagingProxy. Currently this
  // is an empty cross-thread call for the future implementation.
  GetParentFrameTaskRunners()->Get(TaskType::kUnthrottled)
       ->PostTask(
           BLINK_FROM_HERE,
           CrossThreadBind(
                &AudioWorkletMessagingProxy::SynchronizeWorkletProcessorInfo,
                GetAudioWorkletMessagingProxyWeakPtr(),
                WTF::Passed(std::move(info_list))));
}

CrossThreadWeakPersistent<AudioWorkletMessagingProxy>
AudioWorkletObjectProxy::GetAudioWorkletMessagingProxyWeakPtr() {
  return static_cast<AudioWorkletMessagingProxy*>(
      MessagingProxyWeakPtr().Get());
}

}  // namespace blink
