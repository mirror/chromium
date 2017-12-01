// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/run_loop.h"
#include "base/test/scoped_task_environment.h"
#include "components/arc/arc_bridge_service.h"
#include "components/arc/common/power.mojom.h"
#include "components/arc/test/fake_timer_instance.h"
#include "components/arc/timer/arc_timer_bridge.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace arc {

class ArcTimerBridgeTest : public testing::Test {
 public:
  ArcTimerBridgeTest() {
    bridge_service_ = std::make_unique<ArcBridgeService>();
    timer_bridge_ = std::make_unique<ArcTimerBridge>(nullptr /* context */,
                                                     bridge_service_.get());
    CreateTimerInstance();
  }

  ~ArcTimerBridgeTest() override {
    DestroyTimerInstance();
    timer_bridge_.reset();
  }

 protected:
  // Creates a FakeTimerInstance for |bridge_service_|. This results in
  // ArcTimerBridge::OnInstanceReady() being called.
  void CreateTimerInstance() {
    timer_instance_ = std::make_unique<FakeTimerInstance>();
    bridge_service_->timer()->SetInstance(timer_instance_.get(),
                                          mojom::TimerInstance::Version_);
  }

  // Destroys the FakeTimerInstance. This results in
  // ArcTimerBridge::OnInstanceClosed() being called.
  void DestroyTimerInstance() {
    bridge_service_->timer()->SetInstance(nullptr, 0);
    timer_instance_.reset();
  }

  // TODO: Figure out why this is needed.
  base::test::ScopedTaskEnvironment scoped_task_environment_;

  std::unique_ptr<ArcBridgeService> bridge_service_;
  std::unique_ptr<FakeTimerInstance> timer_instance_;
  std::unique_ptr<ArcTimerBridge> timer_bridge_;

 private:
  DISALLOW_COPY_AND_ASSIGN(ArcTimerBridgeTest);
};

TEST_F(ArcTimerBridgeTest, CreateTimers) {
  ASSERT_EQ(0, 0);
  std::vector<clockid_t> clocks = {CLOCK_REALTIME_ALARM, CLOCK_BOOTTIME_ALARM};
  // Create timers using a fake instance and then check if timer corresponding
  // to each clock was created.
  timer_instance_->CallCreateTimers(clocks);
  for (const clockid_t clock : clocks) {
    EXPECT_TRUE(timer_instance_->IsTimerPresent(clock));
  }
}

}  // namespace arc

