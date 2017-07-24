// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef AudioWorkletMessagingProxy_h
#define AudioWorkletMessagingProxy_h

#include <memory>
#include "core/workers/ThreadedWorkletMessagingProxy.h"

namespace blink {

class ExecutionContext;
class WorkerThread;

// AudioWorkletMessagingProxy is a main thread interface for
// AudioWorkletGlobalScope. The proxy communicates with the associated global
// scope via AudioWorkletObjectProxy.
class AudioWorkletMessagingProxy final : public ThreadedWorkletMessagingProxy {
 public:
  AudioWorkletMessagingProxy(ExecutionContext*, WorkerClients*);

 private:
  ~AudioWorkletMessagingProxy() override;

  // Implements ThreadedWorkletMessagingProxy.
  std::unique_ptr<ThreadedWorkletObjectProxy> CreateObjectProxy(
      ThreadedWorkletMessagingProxy*,
      ParentFrameTaskRunners*) override;

  std::unique_ptr<WorkerThread> CreateWorkerThread(double origin_time) override;
};

}  // namespace blink

#endif  // AudioWorkletMessagingProxy_h
