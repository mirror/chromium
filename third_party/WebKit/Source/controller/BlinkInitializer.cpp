// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "controller/BlinkInitializer.h"

#include "bindings/core/v8/V8Initializer.h"
#include "core/animation/AnimationClock.h"
#include "modules/ModulesInitializer.h"
#include "platform/bindings/Microtask.h"
#include "platform/bindings/V8PerIsolateData.h"
#include "platform/heap/Heap.h"
#include "platform/wtf/Assertions.h"
#include "platform/wtf/PtrUtil.h"
#include "platform/wtf/WTF.h"
#include "public/platform/Platform.h"
#include "public/platform/WebThread.h"
#include "v8/include/v8.h"

namespace blink {

namespace {

class EndOfTaskRunner : public WebThread::TaskObserver {
 public:
  void WillProcessTask() override { AnimationClock::NotifyTaskStart(); }
  void DidProcessTask() override {
    Microtask::PerformCheckpoint(V8PerIsolateData::MainThreadIsolate());
    V8Initializer::ReportRejectedPromisesOnMainThread();
  }
};

}  // namespace

static WebThread::TaskObserver* g_end_of_task_runner = nullptr;

static ModulesInitializer& GetModulesInitializer() {
  DEFINE_STATIC_LOCAL(std::unique_ptr<ModulesInitializer>, initializer,
                      (WTF::WrapUnique(new ModulesInitializer)));
  return *initializer;
}

void InitializeBlink(Platform* platform) {
  Platform::Initialize(platform);

  V8Initializer::InitializeMainThread();

  GetModulesInitializer().Initialize();

  // currentThread is null if we are running on a thread without a message loop.
  if (WebThread* current_thread = platform->CurrentThread()) {
    DCHECK(!g_end_of_task_runner);
    g_end_of_task_runner = new EndOfTaskRunner;
    current_thread->AddTaskObserver(g_end_of_task_runner);
  }
}

}  // namespace blink
