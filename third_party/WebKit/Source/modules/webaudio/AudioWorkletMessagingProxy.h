// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef AudioWorkletMessagingProxy_h
#define AudioWorkletMessagingProxy_h

#include <memory>
#include "core/workers/ThreadedWorkletMessagingProxy.h"

namespace blink {

class AudioParamInfo;
class AudioWorkletProcessorInfo;
class ExecutionContext;
class WorkerThread;

// AudioWorkletMessagingProxy is a main thread interface for
// AudioWorkletGlobalScope. The proxy communicates with the associated global
// scope via AudioWorkletObjectProxy.
class AudioWorkletMessagingProxy final : public ThreadedWorkletMessagingProxy {
 public:
  AudioWorkletMessagingProxy(ExecutionContext*, WorkerClients*);

  // Invoked by AudioWorkletObjectProxy to synchronize the information from
  // AudioWorkletGlobalScope after the script code evaluation.
  void SynchronizeWorkletProcessorInfo(
      std::unique_ptr<Vector<AudioWorkletProcessorInfo>>);

  bool IsProcessorRegistered(const String& name);
  const Vector<AudioParamInfo> GetAudioParamInfoForProcessor(
      const String& name) const;

 private:
  ~AudioWorkletMessagingProxy() override;

  // Implements ThreadedWorkletMessagingProxy.
  std::unique_ptr<ThreadedWorkletObjectProxy> CreateObjectProxy(
      ThreadedWorkletMessagingProxy*,
      ParentFrameTaskRunners*) override;

  std::unique_ptr<WorkerThread> CreateWorkerThread() override;
  std::unique_ptr<HashMap<String, Vector<AudioParamInfo>>> processor_info_map_;
};

}  // namespace blink

#endif  // AudioWorkletMessagingProxy_h
