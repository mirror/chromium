// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef AudioWorkletMessagingProxy_h
#define AudioWorkletMessagingProxy_h

#include <memory>
#include "core/workers/ThreadedWorkletMessagingProxy.h"

namespace blink {

class AudioWorkletHandler;
class ExecutionContext;
class WorkerThread;

class AudioWorkletMessagingProxy final : public ThreadedWorkletMessagingProxy {
 public:
  AudioWorkletMessagingProxy(ExecutionContext*, WorkerClients*);

  void CreateProcessor(AudioWorkletHandler*);

  void CreateProcessorOnWorkerThread(AudioWorkletHandler*, WorkerThread*);

 private:
  ~AudioWorkletMessagingProxy() override;

  std::unique_ptr<WorkerThread> CreateWorkerThread() override;
};

}  // namespace blink

#endif  // AudioWorkletMessagingProxy_h
