// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/scheduler/child/worker_scheduler_proxy.h"
#include "base/test/simple_test_tick_clock.h"
#include "components/viz/test/ordered_simple_task_runner.h"
#include "platform/WaitableEvent.h"
#include "platform/scheduler/child/webthread_impl_for_worker_scheduler.h"
#include "platform/scheduler/child/worker_scheduler_impl.h"
#include "platform/scheduler/child/worker_scheduler_proxy_handle.h"
#include "platform/scheduler/renderer/renderer_scheduler_impl.h"
#include "platform/scheduler/renderer/web_frame_scheduler_impl.h"
#include "platform/scheduler/renderer/web_view_scheduler_impl.h"
#include "platform/scheduler/test/create_task_queue_manager_for_test.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace blink {
namespace scheduler {

namespace {

class WorkerSchedulerImplForTest : public WorkerSchedulerImpl {
 public:
  WorkerSchedulerImplForTest(
      std::unique_ptr<TaskQueueManager> manager,
      scoped_refptr<internal::WorkerSchedulerProxy> proxy,
      WaitableEvent* visibility_changed)
      : WorkerSchedulerImpl(std::move(manager), proxy),
        visibility_changed_(visibility_changed) {}

  void OnPageVisible(bool visible) override {
    WorkerSchedulerImpl::OnPageVisible(visible);

    visibility_changed_->Signal();
  }

  using WorkerSchedulerImpl::page_visible;

 private:
  WaitableEvent* visibility_changed_;
};

class WebThreadImplForWorkerSchedulerForTest
    : public WebThreadImplForWorkerScheduler {
 public:
  explicit WebThreadImplForWorkerSchedulerForTest(
      scoped_refptr<internal::WorkerSchedulerProxy> proxy,
      WaitableEvent* visibility_changed)
      : WebThreadImplForWorkerScheduler("Test Worker", base::Thread::Options()),
        proxy_(proxy),
        visibility_changed_(visibility_changed) {}

  std::unique_ptr<WorkerScheduler> CreateWorkerScheduler() override {
    auto scheduler = std::make_unique<WorkerSchedulerImplForTest>(
        TaskQueueManager::TakeOverCurrentThread(), proxy_, visibility_changed_);
    scheduler_ = scheduler.get();
    return scheduler;
  }

  WorkerSchedulerImplForTest* GetWorkerScheduler() { return scheduler_; }

 private:
  scoped_refptr<internal::WorkerSchedulerProxy> proxy_;
  WaitableEvent* visibility_changed_;                // NOT OWNED
  WorkerSchedulerImplForTest* scheduler_ = nullptr;  // NOT OWNED
};

std::unique_ptr<WebThreadImplForWorkerSchedulerForTest> CreateWorkerThread(
    scoped_refptr<internal::WorkerSchedulerProxy> proxy,
    WaitableEvent* visibility_changed) {
  std::unique_ptr<WebThreadImplForWorkerSchedulerForTest> thread =
      std::make_unique<WebThreadImplForWorkerSchedulerForTest>(
          proxy, visibility_changed);
  thread->Init();
  return thread;
}

}  // namespace

class WorkerSchedulerProxyTest : public ::testing::Test {
 public:
  WorkerSchedulerProxyTest()
      : mock_main_thread_task_runner_(
            new cc::OrderedSimpleTaskRunner(&clock_, true)),
        renderer_scheduler_(std::make_unique<RendererSchedulerImpl>(
            CreateTaskQueueManagerForTest(nullptr,
                                          mock_main_thread_task_runner_,
                                          &clock_))),
        web_view_scheduler_(
            std::make_unique<WebViewSchedulerImpl>(nullptr,
                                                   nullptr,
                                                   renderer_scheduler_.get(),
                                                   false)),
        frame_scheduler_(web_view_scheduler_->CreateWebFrameSchedulerImpl(
            nullptr,
            WebFrameScheduler::FrameType::kMainFrame)) {}

  ~WorkerSchedulerProxyTest() {
    frame_scheduler_.reset();
    web_view_scheduler_.reset();
    renderer_scheduler_->Shutdown();
  }

 protected:
  base::SimpleTestTickClock clock_;
  scoped_refptr<cc::OrderedSimpleTaskRunner> mock_main_thread_task_runner_;

  std::unique_ptr<RendererSchedulerImpl> renderer_scheduler_;
  std::unique_ptr<WebViewSchedulerImpl> web_view_scheduler_;
  std::unique_ptr<WebFrameSchedulerImpl> frame_scheduler_;
};

TEST_F(WorkerSchedulerProxyTest, VisibilitySignalReceived) {
  WaitableEvent visibility_changed;

  std::unique_ptr<WorkerSchedulerProxyHandle> proxy_handle =
      frame_scheduler_->CreateWorkerSchedulerProxy();

  auto worker_thread = CreateWorkerThread(
      proxy_handle->GetWorkerSchedulerProxy(), &visibility_changed);

  DCHECK(worker_thread->GetWorkerScheduler()->page_visible());

  frame_scheduler_->SetPageVisible(false);
  visibility_changed.Wait();
  DCHECK(!worker_thread->GetWorkerScheduler()->page_visible());

  frame_scheduler_->SetPageVisible(true);
  visibility_changed.Wait();
  DCHECK(worker_thread->GetWorkerScheduler()->page_visible());

  mock_main_thread_task_runner_->RunUntilIdle();
}

// Tests below check that no crashes occur during different shutdown sequences.

TEST_F(WorkerSchedulerProxyTest, ProxyHandleDestroyed) {
  WaitableEvent visibility_changed;

  std::unique_ptr<WorkerSchedulerProxyHandle> proxy_handle =
      frame_scheduler_->CreateWorkerSchedulerProxy();

  auto worker_thread = CreateWorkerThread(
      proxy_handle->GetWorkerSchedulerProxy(), &visibility_changed);

  DCHECK(worker_thread->GetWorkerScheduler()->page_visible());

  frame_scheduler_->SetPageVisible(false);
  visibility_changed.Wait();
  DCHECK(!worker_thread->GetWorkerScheduler()->page_visible());

  proxy_handle.reset();

  frame_scheduler_->SetPageVisible(true);
  mock_main_thread_task_runner_->RunUntilIdle();

  worker_thread.reset();
  mock_main_thread_task_runner_->RunUntilIdle();

  frame_scheduler_.reset();
  mock_main_thread_task_runner_->RunUntilIdle();
}

TEST_F(WorkerSchedulerProxyTest, FrameSchedulerDestroyed) {
  WaitableEvent visibility_changed;

  std::unique_ptr<WorkerSchedulerProxyHandle> proxy_handle =
      frame_scheduler_->CreateWorkerSchedulerProxy();

  auto worker_thread = CreateWorkerThread(
      proxy_handle->GetWorkerSchedulerProxy(), &visibility_changed);

  DCHECK(worker_thread->GetWorkerScheduler()->page_visible());

  frame_scheduler_->SetPageVisible(false);
  visibility_changed.Wait();
  DCHECK(!worker_thread->GetWorkerScheduler()->page_visible());

  frame_scheduler_.reset();
  mock_main_thread_task_runner_->RunUntilIdle();

  proxy_handle.reset();
  mock_main_thread_task_runner_->RunUntilIdle();

  worker_thread.reset();
  mock_main_thread_task_runner_->RunUntilIdle();
}

TEST_F(WorkerSchedulerProxyTest, ThreadDestroyed) {
  WaitableEvent visibility_changed;

  std::unique_ptr<WorkerSchedulerProxyHandle> proxy_handle =
      frame_scheduler_->CreateWorkerSchedulerProxy();

  auto worker_thread = CreateWorkerThread(
      proxy_handle->GetWorkerSchedulerProxy(), &visibility_changed);

  DCHECK(worker_thread->GetWorkerScheduler()->page_visible());

  frame_scheduler_->SetPageVisible(false);
  visibility_changed.Wait();
  DCHECK(!worker_thread->GetWorkerScheduler()->page_visible());

  worker_thread.reset();

  frame_scheduler_->SetPageVisible(true);
  mock_main_thread_task_runner_->RunUntilIdle();

  frame_scheduler_.reset();
  mock_main_thread_task_runner_->RunUntilIdle();

  proxy_handle.reset();
  mock_main_thread_task_runner_->RunUntilIdle();
}

}  // namespace scheduler
}  // namespace blink
