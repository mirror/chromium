// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/testing/TestingPlatformSupportWithThread.h"

#include "base/bind.h"
#include "base/synchronization/waitable_event.h"
#include "base/threading/thread_local.h"
#include "platform/WaitableEvent.h"
#include "public/platform/scheduler/child/webthread_base.h"

namespace blink {

namespace {

base::ThreadLocalPointer<WebThread>* GetThreadLocalForCurrentThread() {
  static base::ThreadLocalPointer<WebThread> current_thread;
  return &current_thread;
}

void PrepareCurrentThread(base::WaitableEvent* event, WebThread* thread) {
  GetThreadLocalForCurrentThread()->Set(thread);
  event->Signal();
}

}  // namespace

TestingPlatformSupportWithThread::TestingPlatformSupportWithThread() = default;

TestingPlatformSupportWithThread::~TestingPlatformSupportWithThread() = default;

std::unique_ptr<WebThread> TestingPlatformSupportWithThread::CreateThread(
    const char* name) {
  std::unique_ptr<scheduler::WebThreadBase> thread =
      scheduler::WebThreadBase::CreateWorkerThread(name,
                                                   base::Thread::Options());
  thread->Init();
  base::WaitableEvent event(base::WaitableEvent::ResetPolicy::AUTOMATIC,
                            base::WaitableEvent::InitialState::NOT_SIGNALED);
  thread->GetTaskRunner()->PostTask(
      FROM_HERE, base::BindOnce(PrepareCurrentThread, base::Unretained(&event),
                                base::Unretained(thread.get())));
  event.Wait();
  return std::move(thread);
}

WebThread* TestingPlatformSupportWithThread::CurrentThread() {
  WebThread* current_thread = GetThreadLocalForCurrentThread()->Get();
  if (current_thread)
    return current_thread;
  return TestingPlatformSupport::CurrentThread();
}

}  // namespace blink
