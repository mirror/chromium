// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/memory/weak_ptr.h"

#include <memory>
#include <string>

#include "base/bind.h"
#include "base/synchronization/waitable_event.h"
#include "base/test/test_support_ios.h"
#include "base/threading/thread.h"
//#include "testing/gtest/include/gtest/gtest.h"

namespace base {
namespace {

struct TargetBase {};
struct Target : public TargetBase, public SupportsWeakPtr<Target> {
  virtual ~Target() {}
};
struct Arrow {
  WeakPtr<Target> target;
};

// Helper class to create and destroy weak pointer copies
// and delete objects on a background thread.
class BackgroundThread : public Thread {
 public:
  BackgroundThread() : Thread("owner_thread") {}

  ~BackgroundThread() override { Stop(); }

  void CreateArrowFromArrow(Arrow** arrow, const Arrow* other) {
    WaitableEvent completion(WaitableEvent::ResetPolicy::MANUAL,
                             WaitableEvent::InitialState::NOT_SIGNALED);
    task_runner()->PostTask(
        FROM_HERE, base::BindOnce(&BackgroundThread::DoCreateArrowFromArrow,
                                  arrow, other, &completion));
    completion.Wait();
  }

  void DeleteArrow(Arrow* object) {
    WaitableEvent completion(WaitableEvent::ResetPolicy::MANUAL,
                             WaitableEvent::InitialState::NOT_SIGNALED);
    task_runner()->PostTask(
        FROM_HERE,
        base::BindOnce(&BackgroundThread::DoDeleteArrow, object, &completion));
    completion.Wait();
  }

  Target* DeRef(const Arrow* arrow) {
    WaitableEvent completion(WaitableEvent::ResetPolicy::MANUAL,
                             WaitableEvent::InitialState::NOT_SIGNALED);
    Target* result = nullptr;
    task_runner()->PostTask(
        FROM_HERE, base::BindOnce(&BackgroundThread::DoDeRef, arrow, &result,
                                  &completion));
    completion.Wait();
    return result;
  }

 protected:
  static void DoCreateArrowFromArrow(Arrow** arrow,
                                     const Arrow* other,
                                     WaitableEvent* completion) {
    *arrow = new Arrow;
    **arrow = *other;
    completion->Signal();
  }

  static void DoDeRef(const Arrow* arrow,
                      Target** result,
                      WaitableEvent* completion) {
    *result = arrow->target.get();
    completion->Signal();
  }

  static void DoDeleteArrow(Arrow* object, WaitableEvent* completion) {
    delete object;
    completion->Signal();
  }
};

}  // namespace

//TEST(WeakPtrDeathTest, WeakPtrCopyDoesNotChangeThreadBinding) {
void do_the_thing() {
  // The default style "fast" does not support multi-threaded tests
  // (introduces deadlock on Linux).
  //::testing::FLAGS_gtest_death_test_style = "threadsafe";

  BackgroundThread background;
  background.Start();

  // Main thread creates a Target object.
  Target target;
  // Main thread creates an arrow referencing the Target.
  Arrow arrow;
  arrow.target = target.AsWeakPtr();

  // Background copies the WeakPtr.
  Arrow* arrow_copy;
  background.CreateArrowFromArrow(&arrow_copy, &arrow);

  // The copy is still bound to main thread so I can deref.
  //EXPECT_EQ(arrow.target.get(), arrow_copy->target.get());

  background.DeleteArrow(arrow_copy);
}

}  // namespace base

int main() {
  base::RunTestsFromIOSApp();
}
