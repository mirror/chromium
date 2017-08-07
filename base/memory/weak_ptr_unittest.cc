// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/memory/weak_ptr.h"

#include <memory>
#include <string>

#include "base/bind.h"
#include "base/synchronization/waitable_event.h"
#include "base/test/test_support_ios.h"  // nogncheck
#include "base/threading/thread.h"
//#include "testing/gtest/include/gtest/gtest.h"

namespace base {
namespace {

// Helper class to create and destroy weak pointer copies
// and delete objects on a background thread.
class BackgroundThread : public Thread {
 public:
  BackgroundThread() : Thread("owner_thread") {}

  ~BackgroundThread() override { Stop(); }

  static void DoCreateArrowFromArrow(WaitableEvent* completion) {
    completion->Signal();
  }
};

}  // namespace

//TEST(WeakPtrDeathTest, WeakPtrCopyDoesNotChangeThreadBinding) {
void do_the_thing() {
  BackgroundThread background;
  background.Start();

  WaitableEvent completion(WaitableEvent::ResetPolicy::MANUAL,
                           WaitableEvent::InitialState::NOT_SIGNALED);
  background.task_runner()->PostTask(
        FROM_HERE, base::BindOnce(&BackgroundThread::DoCreateArrowFromArrow,
                                  &completion));
  completion.Wait();
}

}  // namespace base

int main(int argc, char* argv[]) {
  base::RunTestsFromIOSApp(argc, argv);
}
