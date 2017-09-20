// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef AnimationWorkletThread_h
#define AnimationWorkletThread_h

#include "modules/ModulesExport.h"
#include "modules/compositorworker/AbstractAnimationWorkletThread.h"
#include <memory>

namespace blink {

class WorkerReportingProxy;

class MODULES_EXPORT AnimationWorkletThread final
    : public AbstractAnimationWorkletThread {
 public:
  static std::unique_ptr<AnimationWorkletThread> Create(
      ThreadableLoadingContext*,
      WorkerReportingProxy&);
  ~AnimationWorkletThread() override;

  WorkerBackingThread& GetWorkerBackingThread() override;

  // The backing thread is cleared by ClearSharedBackingThread().
  void ClearWorkerBackingThread() override {}

  // This may block the main thread.
  static void CollectAllGarbage();

  static void EnsureSharedBackingThread();
  static void ClearSharedBackingThread();

  static void CreateSharedBackingThreadForTest();

  // This only can be called after EnsureSharedBackingThread() is performed.
  // Currently AnimationWorkletThread owns only one thread and it is shared
  // by all the customers.
  static WebThread* GetSharedBackingThread();

 protected:
  WorkerOrWorkletGlobalScope* CreateWorkerGlobalScope(
      std::unique_ptr<GlobalScopeCreationParams>) final;

 private:
  AnimationWorkletThread(ThreadableLoadingContext*, WorkerReportingProxy&);
};

}  // namespace blink

#endif  // AnimationWorkletThread_h
