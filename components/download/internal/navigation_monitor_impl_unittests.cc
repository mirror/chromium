// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/download/internal/navigation_monitor_impl.h"

#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/memory/ptr_util.h"
#include "base/memory/weak_ptr.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/threading/thread_task_runner_handle.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace download {
namespace {

class TestNavigationMonitorObserver : public NavigationMonitor::Observer {
 public:
  TestNavigationMonitorObserver(NavigationMonitor* monitor)
      : monitor_(monitor),
        navigation_in_progress_(false),
        weak_ptr_factory_(this) {}
  ~TestNavigationMonitorObserver() override = default;

  void OnNavigationEvent() override {
    navigation_in_progress_ = monitor_->IsNavigationInProgress();
  }

  void VerifyNavigationState(bool expected) {
    EXPECT_EQ(expected, navigation_in_progress_);
  }

  void VerifyNavigationStateAt(bool expected, int millis) {
    base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
        FROM_HERE,
        base::Bind(&TestNavigationMonitorObserver::VerifyNavigationState,
                   weak_ptr_factory_.GetWeakPtr(), expected),
        base::TimeDelta::FromMilliseconds(millis));
  }

 private:
  NavigationMonitor* monitor_;
  bool navigation_in_progress_;
  base::WeakPtrFactory<TestNavigationMonitorObserver> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(TestNavigationMonitorObserver);
};

class NavigationMonitorImplTest : public testing::Test {
 public:
  NavigationMonitorImplTest() : weak_ptr_factory_(this) {}
  ~NavigationMonitorImplTest() override = default;

  void SetUp() override {
    navigation_monitor_ = base::MakeUnique<NavigationMonitorImpl>();
    observer_ = base::MakeUnique<TestNavigationMonitorObserver>(
        navigation_monitor_.get());
    navigation_monitor_->Configure(base::TimeDelta::FromMilliseconds(20),
                                   base::TimeDelta::FromMilliseconds(200));
  }

  void TearDown() override {
    navigation_monitor_.reset();
    observer_.reset();
  }

  void WaitUntilDone() {
    base::RunLoop run_loop;
    base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
        FROM_HERE, run_loop.QuitClosure(),
        base::TimeDelta::FromMilliseconds(500));
    run_loop.Run();
  }

  void SendNavigationEvent(NavigationEvent event) {
    navigation_monitor_->OnNavigationEvent(event);
  }

  void SendNavigationEventAt(NavigationEvent event, int millis) {
    base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
        FROM_HERE,
        base::Bind(&NavigationMonitorImplTest::SendNavigationEvent,
                   weak_ptr_factory_.GetWeakPtr(), event),
        base::TimeDelta::FromMilliseconds(millis));
  }

 protected:
  base::MessageLoop message_loop_;
  std::unique_ptr<NavigationMonitorImpl> navigation_monitor_;
  std::unique_ptr<TestNavigationMonitorObserver> observer_;
  base::WeakPtrFactory<NavigationMonitorImplTest> weak_ptr_factory_;

 private:
  DISALLOW_COPY_AND_ASSIGN(NavigationMonitorImplTest);
};

TEST_F(NavigationMonitorImplTest, NavigationTimeout) {
  navigation_monitor_->Start(observer_.get());
  SendNavigationEventAt(NavigationEvent::START, 5);

  observer_->VerifyNavigationStateAt(false, 0);
  observer_->VerifyNavigationStateAt(true, 10);
  observer_->VerifyNavigationStateAt(true, 50);
  observer_->VerifyNavigationStateAt(true, 190);
  observer_->VerifyNavigationStateAt(false, 210);
  observer_->VerifyNavigationStateAt(false, 300);
  WaitUntilDone();
}

TEST_F(NavigationMonitorImplTest, UnexpectedNavigationEndCalls) {
  navigation_monitor_->Start(observer_.get());
  SendNavigationEventAt(NavigationEvent::END, 5);
  SendNavigationEventAt(NavigationEvent::END, 10);

  observer_->VerifyNavigationStateAt(false, 0);
  observer_->VerifyNavigationStateAt(false, 7);
  observer_->VerifyNavigationStateAt(false, 15);
  observer_->VerifyNavigationStateAt(false, 50);
  observer_->VerifyNavigationStateAt(false, 300);
  WaitUntilDone();
}

TEST_F(NavigationMonitorImplTest, OverlappingNavigations) {
  navigation_monitor_->Start(observer_.get());
  SendNavigationEventAt(NavigationEvent::START, 5);
  SendNavigationEventAt(NavigationEvent::START, 27);
  SendNavigationEventAt(NavigationEvent::END, 50);
  SendNavigationEventAt(NavigationEvent::END, 55);

  observer_->VerifyNavigationStateAt(false, 0);
  observer_->VerifyNavigationStateAt(true, 20);
  observer_->VerifyNavigationStateAt(true, 40);
  observer_->VerifyNavigationStateAt(true, 60);
  observer_->VerifyNavigationStateAt(false, 80);
  observer_->VerifyNavigationStateAt(false, 300);
  WaitUntilDone();
}

TEST_F(NavigationMonitorImplTest, TwoNavigationsShortlyOneAfterAnother) {
  navigation_monitor_->Start(observer_.get());
  SendNavigationEventAt(NavigationEvent::START, 5);
  SendNavigationEventAt(NavigationEvent::END, 10);
  SendNavigationEventAt(NavigationEvent::START, 27);
  SendNavigationEventAt(NavigationEvent::END, 50);

  observer_->VerifyNavigationStateAt(false, 0);
  observer_->VerifyNavigationStateAt(true, 7);
  observer_->VerifyNavigationStateAt(true, 20);
  observer_->VerifyNavigationStateAt(true, 40);
  observer_->VerifyNavigationStateAt(true, 60);
  observer_->VerifyNavigationStateAt(false, 80);
  observer_->VerifyNavigationStateAt(false, 300);
  WaitUntilDone();
}

TEST_F(NavigationMonitorImplTest, NavigationSpacedApartLongTime) {
  navigation_monitor_->Start(observer_.get());
  SendNavigationEventAt(NavigationEvent::START, 5);
  SendNavigationEventAt(NavigationEvent::END, 10);
  SendNavigationEventAt(NavigationEvent::START, 60);
  SendNavigationEventAt(NavigationEvent::END, 70);

  observer_->VerifyNavigationStateAt(false, 0);
  observer_->VerifyNavigationStateAt(true, 7);
  observer_->VerifyNavigationStateAt(true, 15);
  observer_->VerifyNavigationStateAt(false, 40);
  observer_->VerifyNavigationStateAt(true, 65);
  observer_->VerifyNavigationStateAt(true, 80);
  observer_->VerifyNavigationStateAt(false, 100);
  WaitUntilDone();
}

}  // namespace
}  // namespace download
