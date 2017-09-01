// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/download/internal/navigation_monitor_impl.h"

#include "base/memory/ptr_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace download {
namespace {

class MockNavigationMonitorObserver : public NavigationMonitor::Observer {
 public:
  MockNavigationMonitorObserver() = default;
  ~MockNavigationMonitorObserver() override = default;
  MOCK_METHOD1(OnNavigationEvent, void(NavigationEvent));
};

class NavigationMonitorImplTest : public testing::Test {
 public:
  NavigationMonitorImplTest() = default;
  ~NavigationMonitorImplTest() override = default;

  void SetUp() override {
    navigation_monitor_ = base::MakeUnique<NavigationMonitorImpl>();
    observer_ = base::MakeUnique<MockNavigationMonitorObserver>();
  }

  void TearDown() override {
    navigation_monitor_.reset();
    observer_.reset();
  }

 protected:
  std::unique_ptr<NavigationMonitorImpl> navigation_monitor_;
  std::unique_ptr<MockNavigationMonitorObserver> observer_;

 private:
  DISALLOW_COPY_AND_ASSIGN(NavigationMonitorImplTest);
};

// Ensures the navigation events are piped to the observer.
TEST_F(NavigationMonitorImplTest, NavigationEventPipe) {
  navigation_monitor_->Start(observer_.get());
  EXPECT_CALL(*observer_.get(), OnNavigationEvent(NavigationEvent::START))
      .Times(1)
      .RetiresOnSaturation();
  navigation_monitor_->OnNavigationEvent(NavigationEvent::START);

  EXPECT_CALL(*observer_.get(), OnNavigationEvent(NavigationEvent::END))
      .Times(1)
      .RetiresOnSaturation();
  navigation_monitor_->OnNavigationEvent(NavigationEvent::END);
}

}  // namespace
}  // namespace download
