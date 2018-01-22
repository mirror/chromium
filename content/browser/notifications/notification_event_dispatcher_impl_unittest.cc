// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/notifications/notification_event_dispatcher_impl.h"

#include <stdint.h>
#include <memory>
#include <vector>

#include "base/macros.h"
#include "base/test/test_simple_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/WebKit/public/platform/modules/notifications/notification_service.mojom.h"

namespace content {

const char kPrimaryUniqueId[] = "this_should_be_a_unique_id";
const char kSomeOtherUniqueId[] = "and_this_one_is_different_and_also_unique";

class TestNotificationListener
    : public blink::mojom::NonPersistentNotificationListener {
 public:
  TestNotificationListener() : binding_(this) {}
  ~TestNotificationListener() override = default;

  // Closes the bindings associated with this listener.
  void Close() { binding_.Close(); }

  // Returns an InterfacePtr to this listener.
  blink::mojom::NonPersistentNotificationListenerPtr GetPtr() {
    blink::mojom::NonPersistentNotificationListenerPtr ptr;
    binding_.Bind(mojo::MakeRequest(&ptr));
    return ptr;
  }

  // Returns the number of OnShow events received by this listener.
  int on_show_count() const { return on_show_count_; }

  // blink::mojom::NonPersistentNotificationListener implementation.
  void OnShow() override { on_show_count_++; }

 private:
  int on_show_count_ = 0;
  mojo::Binding<blink::mojom::NonPersistentNotificationListener> binding_;

  DISALLOW_COPY_AND_ASSIGN(TestNotificationListener);
};

class NotificationEventDispatcherImplTest : public ::testing::Test {
 public:
  NotificationEventDispatcherImplTest()
      : task_runner_(new base::TestSimpleTaskRunner),
        handle_(task_runner_),
        // Can't use std::make_unique because the constructor is private.
        dispatcher_(base::WrapUnique(new NotificationEventDispatcherImpl())) {}

  ~NotificationEventDispatcherImplTest() override = default;

  // Waits until the task runner managing the Mojo connection has finished.
  void WaitForMojoTasksToComplete() { task_runner_->RunUntilIdle(); }

 protected:
  scoped_refptr<base::TestSimpleTaskRunner> task_runner_;
  base::ThreadTaskRunnerHandle handle_;
  std::unique_ptr<NotificationEventDispatcherImpl> dispatcher_;

 private:
  DISALLOW_COPY_AND_ASSIGN(NotificationEventDispatcherImplTest);
};

TEST_F(NotificationEventDispatcherImplTest,
       DispatchNonPersistentShowEvent_NotifiesCorrectRegisteredListener) {
  auto listener = std::make_unique<TestNotificationListener>();
  dispatcher_->RegisterNonPersistentNotificationListener(kPrimaryUniqueId,
                                                         listener->GetPtr());
  auto other_listener = std::make_unique<TestNotificationListener>();
  dispatcher_->RegisterNonPersistentNotificationListener(
      kSomeOtherUniqueId, other_listener->GetPtr());

  dispatcher_->DispatchNonPersistentShowEvent(kPrimaryUniqueId);

  WaitForMojoTasksToComplete();

  EXPECT_EQ(listener->on_show_count(), 1);
  EXPECT_EQ(other_listener->on_show_count(), 0);

  dispatcher_->DispatchNonPersistentShowEvent(kSomeOtherUniqueId);

  WaitForMojoTasksToComplete();

  EXPECT_EQ(listener->on_show_count(), 1);
  EXPECT_EQ(other_listener->on_show_count(), 1);
}

}  // namespace content
