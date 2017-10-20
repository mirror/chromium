// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/oom_intervention/near_oom_monitor.h"

#include "base/message_loop/message_loop.h"
#include "base/test/test_mock_time_task_runner.h"
#include "testing/gtest/include/gtest/gtest.h"

class MockNearOomMonitor : public NearOomMonitor {
 public:
  explicit MockNearOomMonitor(
      scoped_refptr<base::SequencedTaskRunner> task_runner) {
    SetTaskRunner(task_runner);

    // Start with 128MB swap total and 64MB swap free.
    memory_info_.swap_total = 128 * 1024;
    memory_info_.swap_free = 64 * 1024;
  }

  void SetSwapTotal(int swap_total) { memory_info_.swap_total = swap_total; }

  void SetSwapFree(int swap_free) { memory_info_.swap_free = swap_free; }

 private:
  bool GetSystemMemoryInfo(base::SystemMemoryInfoKB* memory_info) override {
    memcpy(memory_info, &memory_info_, sizeof(memory_info_));
    return true;
  }

  base::SystemMemoryInfoKB memory_info_;
};

class MockNearOomObserver : public NearOomObserver {
 public:
  void OnNearOomDetected() override { is_detected_ = true; }

  bool is_detected() const { return is_detected_; }

 private:
  bool is_detected_ = false;
};

class NearOomMonitorTest : public testing::Test {
 public:
  void SetUp() override {
    task_runner_ = new base::TestMockTimeTaskRunner();
    monitor_.reset(new MockNearOomMonitor(task_runner_));
  }

  void RunUntilIdle() {
    task_runner_->RunUntilIdle();
    // Need to use RunLoop because NearOomMonitor uses ObserverListThreadSafe
    // internally.
    base::RunLoop loop;
    loop.RunUntilIdle();
  }

 protected:
  scoped_refptr<base::TestMockTimeTaskRunner> task_runner_;
  base::MessageLoop message_loop_;
  std::unique_ptr<MockNearOomMonitor> monitor_;
};

TEST_F(NearOomMonitorTest, NoSwap) {
  monitor_->SetSwapTotal(0);
  EXPECT_FALSE(monitor_->Start());
  EXPECT_FALSE(monitor_->IsRunning());
}

TEST_F(NearOomMonitorTest, Observe) {
  ASSERT_TRUE(monitor_->Start());
  ASSERT_TRUE(monitor_->IsRunning());

  MockNearOomObserver observer1;
  monitor_->Register(&observer1);
  MockNearOomObserver observer2;
  monitor_->Register(&observer2);
  RunUntilIdle();

  base::TimeDelta interval =
      monitor_->GetMonitoringInterval() + base::TimeDelta::FromSeconds(1);

  task_runner_->FastForwardBy(interval);
  RunUntilIdle();
  EXPECT_FALSE(observer1.is_detected());
  EXPECT_FALSE(observer2.is_detected());

  monitor_->Unregister(&observer2);
  RunUntilIdle();

  // Simulate near-OOM situation by setting swap free to zero.
  monitor_->SetSwapFree(0);
  task_runner_->FastForwardBy(interval);
  RunUntilIdle();
  EXPECT_TRUE(observer1.is_detected());
  EXPECT_FALSE(observer2.is_detected());

  monitor_->Unregister(&observer1);
  RunUntilIdle();
}
