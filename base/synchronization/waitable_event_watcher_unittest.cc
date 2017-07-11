// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/synchronization/waitable_event_watcher.h"

#include "base/bind.h"
#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/synchronization/waitable_event.h"
#include "base/test/scoped_task_environment.h"
#include "base/threading/platform_thread.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "base/threading/simple_thread.h"
#include "build/build_config.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace base {

namespace {

// The message loops on which each waitable event timer should be tested.
const MessageLoop::Type testing_message_loops[] = {
  MessageLoop::TYPE_DEFAULT,
  MessageLoop::TYPE_IO,
#if !defined(OS_IOS)  // iOS does not allow direct running of the UI loop.
  MessageLoop::TYPE_UI,
#endif
};

void QuitWhenSignaled(WaitableEvent* event) {
  MessageLoop::current()->QuitWhenIdle();
}

class DecrementCountContainer {
 public:
  explicit DecrementCountContainer(int* counter) : counter_(counter) {}
  void OnWaitableEventSignaled(WaitableEvent* object) {
    // NOTE: |object| may be already deleted.
    --(*counter_);
  }

 private:
  int* counter_;
};

}  // namespace

class WaitableEventWatcherTest
    : public testing::TestWithParam<MessageLoop::Type> {
 public:
  static test::ScopedTaskEnvironment::MainThreadType
  MainThreadTypeForMessageLoopType(MessageLoop::Type type) {
    switch (type) {
      case MessageLoop::Type::TYPE_UI:
        return test::ScopedTaskEnvironment::MainThreadType::UI;
      case MessageLoop::Type::TYPE_IO:
        return test::ScopedTaskEnvironment::MainThreadType::IO;
      default:
        return test::ScopedTaskEnvironment::MainThreadType::DEFAULT;
    }
  }

 protected:
  test::ScopedTaskEnvironment::MainThreadType MainThreadType() {
    return MainThreadTypeForMessageLoopType(GetParam());
  }

 private:
  test::ScopedTaskEnvironment scoped_task_environment_{MainThreadType()};
};

TEST_P(WaitableEventWatcherTest, BasicSignal) {
  // A manual-reset event that is not yet signaled.
  WaitableEvent event(WaitableEvent::ResetPolicy::MANUAL,
                      WaitableEvent::InitialState::NOT_SIGNALED);

  WaitableEventWatcher watcher;
  watcher.StartWatching(&event, BindOnce(&QuitWhenSignaled));

  event.Signal();

  RunLoop().Run();
}

TEST_P(WaitableEventWatcherTest, BasicCancel) {
  // A manual-reset event that is not yet signaled.
  WaitableEvent event(WaitableEvent::ResetPolicy::MANUAL,
                      WaitableEvent::InitialState::NOT_SIGNALED);

  WaitableEventWatcher watcher;

  watcher.StartWatching(&event, BindOnce(&QuitWhenSignaled));

  watcher.StopWatching();
}

TEST_P(WaitableEventWatcherTest, CancelAfterSet) {
  // A manual-reset event that is not yet signaled.
  WaitableEvent event(WaitableEvent::ResetPolicy::MANUAL,
                      WaitableEvent::InitialState::NOT_SIGNALED);

  WaitableEventWatcher watcher;

  int counter = 1;
  DecrementCountContainer delegate(&counter);
  WaitableEventWatcher::EventCallback callback = BindOnce(
      &DecrementCountContainer::OnWaitableEventSignaled, Unretained(&delegate));
  watcher.StartWatching(&event, std::move(callback));

  event.Signal();

  // Let the background thread do its business
  base::PlatformThread::Sleep(base::TimeDelta::FromMilliseconds(30));

  watcher.StopWatching();

  RunLoop().RunUntilIdle();

  // Our delegate should not have fired.
  EXPECT_EQ(1, counter);
}

// Simulate a MessageLoop that dies before an WaitableEventWatcher.  This
// ordinarily doesn't happen when people use the Thread class, but it can
// happen when people use the Singleton pattern or atexit.
//
// This test runs on a dedicated SipmleThread, so that it can control the
// precise destruction order of the MessageLoop. This isn't possible on the
// test main thread, since the ScopedTaskEnvironment owns it.
class OutlivesMessageLoopThread : public SimpleThread {
 public:
  explicit OutlivesMessageLoopThread(MessageLoop::Type message_loop_type)
      : SimpleThread("WaitableEvent.OutlivesMessageLoopThread"),
        message_loop_type_(message_loop_type) {}

  void Run() override {
    WaitableEvent event(WaitableEvent::ResetPolicy::MANUAL,
                        WaitableEvent::InitialState::NOT_SIGNALED);
    {
      WaitableEventWatcher watcher;
      {
        MessageLoop message_loop(message_loop_type_);

        watcher.StartWatching(&event, BindOnce(&QuitWhenSignaled));
      }
    }

    did_run_ = true;
  }

  bool did_run() const { return did_run_; }

 private:
  const MessageLoop::Type message_loop_type_;
  bool did_run_ = false;

  DISALLOW_COPY_AND_ASSIGN(OutlivesMessageLoopThread);
};

TEST_P(WaitableEventWatcherTest, OutlivesMessageLoop) {
  OutlivesMessageLoopThread test_thread(GetParam());
  test_thread.Start();
  EXPECT_TRUE(test_thread.HasBeenStarted());
  test_thread.Join();
  EXPECT_TRUE(test_thread.did_run());
}

TEST_P(WaitableEventWatcherTest, SignaledAtStart) {
  WaitableEvent event(WaitableEvent::ResetPolicy::MANUAL,
                      WaitableEvent::InitialState::SIGNALED);

  WaitableEventWatcher watcher;
  watcher.StartWatching(&event, BindOnce(&QuitWhenSignaled));

  RunLoop().Run();
}

TEST_P(WaitableEventWatcherTest, StartWatchingInCallback) {
  WaitableEvent event(WaitableEvent::ResetPolicy::MANUAL,
                      WaitableEvent::InitialState::NOT_SIGNALED);

  WaitableEventWatcher watcher;
  watcher.StartWatching(
      &event, BindOnce(
                  [](WaitableEventWatcher* watcher, WaitableEvent* event) {
                    // |event| is manual, so the second watcher will run
                    // immediately.
                    watcher->StartWatching(event, BindOnce(&QuitWhenSignaled));
                  },
                  &watcher));

  event.Signal();

  RunLoop().Run();
}

// To help detect errors around deleting WaitableEventWatcher, an additional
// bool parameter is used to test sleeping between watching and deletion.
class WaitableEventWatcherDeletionTest
    : public testing::TestWithParam<std::tuple<MessageLoop::Type, bool>> {
 private:
  test::ScopedTaskEnvironment scoped_task_environment_{
      WaitableEventWatcherTest::MainThreadTypeForMessageLoopType(
          std::get<0>(GetParam()))};
};

TEST_P(WaitableEventWatcherDeletionTest, DeleteUnder) {
  bool delay_after_delete = std::get<1>(GetParam());

  // Delete the WaitableEvent out from under the Watcher. This is explictly
  // allowed by the interface.

  {
    WaitableEventWatcher watcher;

    auto* event = new WaitableEvent(WaitableEvent::ResetPolicy::AUTOMATIC,
                                    WaitableEvent::InitialState::NOT_SIGNALED);

    watcher.StartWatching(event, BindOnce(&QuitWhenSignaled));

    if (delay_after_delete) {
      // On Windows that sleep() improves the chance to catch some problems.
      // It postpones the dtor |watcher| (which immediately cancel the waiting)
      // and gives some time to run to a created background thread.
      // Unfortunately, that thread is under OS control and we can't
      // manipulate it directly.
      base::PlatformThread::Sleep(base::TimeDelta::FromMilliseconds(30));
    }

    delete event;
  }
}

TEST_P(WaitableEventWatcherDeletionTest, SignalAndDelete) {
  bool delay_after_delete = std::get<1>(GetParam());

  // Signal and immediately delete the WaitableEvent out from under the Watcher.

  {
    WaitableEventWatcher watcher;

    auto event = base::MakeUnique<WaitableEvent>(
        WaitableEvent::ResetPolicy::AUTOMATIC,
        WaitableEvent::InitialState::NOT_SIGNALED);

    watcher.StartWatching(event.get(), BindOnce(&QuitWhenSignaled));
    event->Signal();
    event.reset();

    if (delay_after_delete) {
      // On Windows that sleep() improves the chance to catch some problems.
      // It postpones the dtor |watcher| (which immediately cancel the waiting)
      // and gives some time to run to a created background thread.
      // Unfortunately, that thread is under OS control and we can't
      // manipulate it directly.
      base::PlatformThread::Sleep(base::TimeDelta::FromMilliseconds(30));
    }

    // Wait for the watcher callback.
    RunLoop().Run();
  }
}

INSTANTIATE_TEST_CASE_P(,
                        WaitableEventWatcherTest,
                        testing::ValuesIn(testing_message_loops));

INSTANTIATE_TEST_CASE_P(
    ,
    WaitableEventWatcherDeletionTest,
    testing::Combine(testing::ValuesIn(testing_message_loops),
                     testing::Bool()));

}  // namespace base
