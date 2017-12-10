// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/run_loop.h"
#include "base/test/scoped_task_environment.h"
#include "base/time/time.h"
#include "components/arc/arc_bridge_service.h"
#include "components/arc/arc_service_manager.h"
#include "components/arc/common/timer.mojom.h"
#include "components/arc/connection_holder.h"
#include "components/arc/test/connection_holder_util.h"
#include "components/arc/test/fake_timer_instance.h"
#include "components/arc/test/test_browser_context.h"
#include "components/arc/timer/arc_timer_bridge.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace arc {

namespace {

ArcTimerBridge* GetArcTimerBridge(content::BrowserContext* context) {
  ArcTimerBridge::GetFactory()->SetTestingFactoryAndUse(
      context,
      [](content::BrowserContext* context) -> std::unique_ptr<KeyedService> {
        return std::make_unique<ArcTimerBridge>(
            context, ArcServiceManager::Get()->arc_bridge_service());
      });
  return ArcTimerBridge::GetForBrowserContext(context);
}

class ArcTimerTest : public testing::Test {
 public:
  ArcTimerTest()
      : scoped_task_environment_(
            base::test::ScopedTaskEnvironment::MainThreadType::IO),
        arc_service_manager_(std::make_unique<ArcServiceManager>()),
        context_(std::make_unique<TestBrowserContext>()),
        timer_instance_(std::make_unique<FakeTimerInstance>()),
        timer_bridge_(GetArcTimerBridge(context_.get())) {
    LOG(ERROR) << "Constructor";
    // This results in ArcTimerBridge::OnInstanceReady being called.
    ArcServiceManager::Get()->arc_bridge_service()->timer()->SetInstance(
        timer_instance_.get());
    WaitForInstanceReady(
        ArcServiceManager::Get()->arc_bridge_service()->timer());
  }

  ~ArcTimerTest() override {
    // Destroys the FakeTimerInstance. This results in
    // ArcTimerBridge::OnInstanceClosed being called.
    ArcServiceManager::Get()->arc_bridge_service()->timer()->CloseInstance(
        timer_instance_.get());
    timer_bridge_->Shutdown();
  }

 protected:
  FakeTimerInstance* GetFakeTimerInstance() { return timer_instance_.get(); }

 private:
  base::test::ScopedTaskEnvironment scoped_task_environment_;

  std::unique_ptr<ArcServiceManager> arc_service_manager_;
  std::unique_ptr<TestBrowserContext> context_;
  std::unique_ptr<FakeTimerInstance> timer_instance_;
  ArcTimerBridge* const timer_bridge_;

  DISALLOW_COPY_AND_ASSIGN(ArcTimerTest);
};

}  // namespace

TEST_F(ArcTimerTest, CreateAndStartTimers) {
  base::FileDescriptorWatcher file_descriptor_watcher(
      base::MessageLoopForIO::current());
  std::vector<clockid_t> clocks = {CLOCK_REALTIME_ALARM, CLOCK_BOOTTIME_ALARM};
  // Create timers using a fake instance and then check if timer corresponding
  // to each clock was created.
  GetFakeTimerInstance()->CallCreateTimers(clocks);
  for (const clockid_t clock : clocks) {
    EXPECT_TRUE(GetFakeTimerInstance()->IsTimerPresent(clock));
  }
  EXPECT_TRUE(GetFakeTimerInstance()->CallStartTimer(
      CLOCK_BOOTTIME_ALARM, 5 * base::Time::kNanosecondsPerSecond));
  EXPECT_TRUE(GetFakeTimerInstance()->WaitForExpiration(CLOCK_BOOTTIME_ALARM));
}

}  // namespace arc
