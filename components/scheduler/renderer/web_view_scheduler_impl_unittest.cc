// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/scheduler/renderer/web_view_scheduler_impl.h"

#include "base/callback.h"
#include "base/memory/scoped_ptr.h"
#include "base/test/simple_test_tick_clock.h"
#include "cc/test/ordered_simple_task_runner.h"
#include "components/scheduler/base/test_time_source.h"
#include "components/scheduler/child/scheduler_tqm_delegate_for_test.h"
#include "components/scheduler/renderer/renderer_scheduler_impl.h"
#include "components/scheduler/renderer/web_frame_scheduler_impl.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/WebKit/public/platform/WebTaskRunner.h"
#include "third_party/WebKit/public/platform/WebTraceLocation.h"

using testing::ElementsAre;

namespace scheduler {

class WebViewSchedulerImplTest : public testing::Test {
 public:
  WebViewSchedulerImplTest() {}
  ~WebViewSchedulerImplTest() override {}

  void SetUp() override {
    clock_.reset(new base::SimpleTestTickClock());
    clock_->Advance(base::TimeDelta::FromMicroseconds(5000));
    mock_task_runner_ =
        make_scoped_refptr(new cc::OrderedSimpleTaskRunner(clock_.get(), true));
    delagate_ = SchedulerTqmDelegateForTest::Create(
        mock_task_runner_, make_scoped_ptr(new TestTimeSource(clock_.get())));
    scheduler_.reset(new RendererSchedulerImpl(delagate_));
    web_view_scheduler_.reset(new WebViewSchedulerImpl(
        nullptr, scheduler_.get(), DisableBackgroundTimerThrottling()));
    web_frame_scheduler_ = web_view_scheduler_->createWebFrameSchedulerImpl();
  }

  void TearDown() override {
    web_frame_scheduler_.reset();
    web_view_scheduler_.reset();
    scheduler_->Shutdown();
    scheduler_.reset();
  }

  virtual bool DisableBackgroundTimerThrottling() const { return false; }

  scoped_ptr<base::SimpleTestTickClock> clock_;
  scoped_refptr<cc::OrderedSimpleTaskRunner> mock_task_runner_;
  scoped_refptr<SchedulerTqmDelegate> delagate_;
  scoped_ptr<RendererSchedulerImpl> scheduler_;
  scoped_ptr<WebViewSchedulerImpl> web_view_scheduler_;
  scoped_ptr<WebFrameSchedulerImpl> web_frame_scheduler_;
};

TEST_F(WebViewSchedulerImplTest, TestDestructionOfFrameSchedulersBefore) {
  blink::WebPassOwnPtr<blink::WebFrameScheduler> frame1(
      web_view_scheduler_->createFrameScheduler());
  blink::WebPassOwnPtr<blink::WebFrameScheduler> frame2(
      web_view_scheduler_->createFrameScheduler());
}

TEST_F(WebViewSchedulerImplTest, TestDestructionOfFrameSchedulersAfter) {
  blink::WebPassOwnPtr<blink::WebFrameScheduler> frame1(
      web_view_scheduler_->createFrameScheduler());
  blink::WebPassOwnPtr<blink::WebFrameScheduler> frame2(
      web_view_scheduler_->createFrameScheduler());
  web_view_scheduler_.reset();
}

namespace {
class RepeatingTask : public blink::WebTaskRunner::Task {
 public:
  RepeatingTask(blink::WebTaskRunner* web_task_runner, int* run_count)
      : web_task_runner_(web_task_runner), run_count_(run_count) {}

  ~RepeatingTask() override {}

  void run() override {
    (*run_count_)++;
    web_task_runner_->postDelayedTask(
        BLINK_FROM_HERE, new RepeatingTask(web_task_runner_, run_count_), 1.0);
  }

 private:
  blink::WebTaskRunner* web_task_runner_;  // NOT OWNED
  int* run_count_;                         // NOT OWNED
};
}  // namespace

TEST_F(WebViewSchedulerImplTest, RepeatingTimer_PageInForeground) {
  web_view_scheduler_->setPageVisible(true);

  int run_count = 0;
  web_frame_scheduler_->timerTaskRunner()->postDelayedTask(
      BLINK_FROM_HERE,
      new RepeatingTask(web_frame_scheduler_->timerTaskRunner(), &run_count),
      1.0);

  mock_task_runner_->RunForPeriod(base::TimeDelta::FromSeconds(1));
  EXPECT_EQ(1000, run_count);
}

TEST_F(WebViewSchedulerImplTest, RepeatingTimer_PageInBackground) {
  web_view_scheduler_->setPageVisible(false);

  int run_count = 0;
  web_frame_scheduler_->timerTaskRunner()->postDelayedTask(
      BLINK_FROM_HERE,
      new RepeatingTask(web_frame_scheduler_->timerTaskRunner(), &run_count),
      1.0);

  mock_task_runner_->RunForPeriod(base::TimeDelta::FromSeconds(1));
  EXPECT_EQ(1, run_count);
}

TEST_F(WebViewSchedulerImplTest, RepeatingLoadingTask_PageInBackground) {
  web_view_scheduler_->setPageVisible(false);

  int run_count = 0;
  web_frame_scheduler_->loadingTaskRunner()->postDelayedTask(
      BLINK_FROM_HERE,
      new RepeatingTask(web_frame_scheduler_->loadingTaskRunner(), &run_count),
      1.0);

  mock_task_runner_->RunForPeriod(base::TimeDelta::FromSeconds(1));
  EXPECT_EQ(1000, run_count);  // Loading tasks should not be throttled
}

TEST_F(WebViewSchedulerImplTest, RepeatingTimers_OneBackgroundOneForeground) {
  scoped_ptr<WebViewSchedulerImpl> web_view_scheduler2(
      new WebViewSchedulerImpl(nullptr, scheduler_.get(), false));
  scoped_ptr<WebFrameSchedulerImpl> web_frame_scheduler2 =
      web_view_scheduler2->createWebFrameSchedulerImpl();

  web_view_scheduler_->setPageVisible(true);
  web_view_scheduler2->setPageVisible(false);

  int run_count1 = 0;
  int run_count2 = 0;
  web_frame_scheduler_->timerTaskRunner()->postDelayedTask(
      BLINK_FROM_HERE,
      new RepeatingTask(web_frame_scheduler_->timerTaskRunner(), &run_count1),
      1.0);
  web_frame_scheduler2->timerTaskRunner()->postDelayedTask(
      BLINK_FROM_HERE,
      new RepeatingTask(web_frame_scheduler2->timerTaskRunner(), &run_count2),
      1.0);

  mock_task_runner_->RunForPeriod(base::TimeDelta::FromSeconds(1));
  EXPECT_EQ(1000, run_count1);
  EXPECT_EQ(1, run_count2);
}

namespace {
class VirtualTimeRecorderTask : public blink::WebTaskRunner::Task {
 public:
  VirtualTimeRecorderTask(RendererSchedulerImpl* scheduler,
                          std::vector<base::TimeTicks>* out_real_times,
                          std::vector<size_t>* out_virtual_times_ms)
      : scheduler_(scheduler),
        out_real_times_(out_real_times),
        out_virtual_times_ms_(out_virtual_times_ms) {}

  ~VirtualTimeRecorderTask() override {}

  void run() override {
    out_real_times_->push_back(scheduler_->tick_clock()->NowTicks());
    out_virtual_times_ms_->push_back(
        scheduler_->MonotonicallyIncreasingVirtualTimeSeconds() * 1000.0);
  }

 private:
  RendererSchedulerImpl* scheduler_;              // NOT OWNED
  std::vector<base::TimeTicks>* out_real_times_;  // NOT OWNED
  std::vector<size_t>* out_virtual_times_ms_;     // NOT OWNED
};
}

TEST_F(WebViewSchedulerImplTest, VirtualTime_TimerFastForwarding) {
  std::vector<base::TimeTicks> real_times;
  std::vector<size_t> virtual_times_ms;
  base::TimeTicks initial_real_time = scheduler_->tick_clock()->NowTicks();
  size_t initial_virtual_time_ms =
      scheduler_->MonotonicallyIncreasingVirtualTimeSeconds() * 1000.0;

  web_view_scheduler_->enableVirtualTime();

  web_frame_scheduler_->timerTaskRunner()->postDelayedTask(
      BLINK_FROM_HERE,
      new VirtualTimeRecorderTask(scheduler_.get(), &real_times,
                                  &virtual_times_ms),
      2.0);

  web_frame_scheduler_->timerTaskRunner()->postDelayedTask(
      BLINK_FROM_HERE,
      new VirtualTimeRecorderTask(scheduler_.get(), &real_times,
                                  &virtual_times_ms),
      20.0);

  web_frame_scheduler_->timerTaskRunner()->postDelayedTask(
      BLINK_FROM_HERE,
      new VirtualTimeRecorderTask(scheduler_.get(), &real_times,
                                  &virtual_times_ms),
      200.0);

  mock_task_runner_->RunUntilIdle();

  EXPECT_THAT(real_times, ElementsAre(initial_real_time,
                                      initial_real_time, initial_real_time));
  EXPECT_THAT(virtual_times_ms,
              ElementsAre(initial_virtual_time_ms + 2,
                          initial_virtual_time_ms + 20,
                          initial_virtual_time_ms + 200));
}

TEST_F(WebViewSchedulerImplTest, VirtualTime_LoadingTaskFastForwarding) {
  std::vector<base::TimeTicks> real_times;
  std::vector<size_t> virtual_times_ms;
  base::TimeTicks initial_real_time = scheduler_->tick_clock()->NowTicks();
  size_t initial_virtual_time_ms =
      scheduler_->MonotonicallyIncreasingVirtualTimeSeconds() * 1000.0;

  web_view_scheduler_->enableVirtualTime();

  web_frame_scheduler_->loadingTaskRunner()->postDelayedTask(
      BLINK_FROM_HERE,
      new VirtualTimeRecorderTask(scheduler_.get(), &real_times,
                                  &virtual_times_ms),
      2.0);

  web_frame_scheduler_->loadingTaskRunner()->postDelayedTask(
      BLINK_FROM_HERE,
      new VirtualTimeRecorderTask(scheduler_.get(), &real_times,
                                  &virtual_times_ms),
      20.0);

  web_frame_scheduler_->loadingTaskRunner()->postDelayedTask(
      BLINK_FROM_HERE,
      new VirtualTimeRecorderTask(scheduler_.get(), &real_times,
                                  &virtual_times_ms),
      200.0);

  mock_task_runner_->RunUntilIdle();

  EXPECT_THAT(real_times, ElementsAre(initial_real_time,
                                      initial_real_time, initial_real_time));
  EXPECT_THAT(virtual_times_ms,
              ElementsAre(initial_virtual_time_ms + 2,
                          initial_virtual_time_ms + 20,
                          initial_virtual_time_ms + 200));
}

TEST_F(WebViewSchedulerImplTest,
       RepeatingTimer_PageInBackground_MeansNothingForFirtualTime) {
  web_view_scheduler_->enableVirtualTime();
  web_view_scheduler_->setPageVisible(false);
  base::TimeTicks initial_real_time = scheduler_->tick_clock()->NowTicks();

  int run_count = 0;
  web_frame_scheduler_->timerTaskRunner()->postDelayedTask(
      BLINK_FROM_HERE,
      new RepeatingTask(web_frame_scheduler_->timerTaskRunner(), &run_count),
      1.0);

  mock_task_runner_->RunTasksWhile(mock_task_runner_->TaskRunCountBelow(2000));
  EXPECT_EQ(1999, run_count);

  // The global tick clock has not moved, yet we ran a large number of "delayed"
  // tasks despite calling setPageVisible(false).
  EXPECT_EQ(initial_real_time, scheduler_->tick_clock()->NowTicks());
}

namespace {
class RunOrderTask : public blink::WebTaskRunner::Task {
 public:
  RunOrderTask(int index, std::vector<int>* out_run_order)
      : index_(index),
        out_run_order_(out_run_order) {}

  ~RunOrderTask() override {}

  void run() override {
    out_run_order_->push_back(index_);
  }

 private:
  int index_;
  std::vector<int>* out_run_order_;  // NOT OWNED
};

class DelayedRunOrderTask : public blink::WebTaskRunner::Task {
 public:
  DelayedRunOrderTask(int index, blink::WebTaskRunner* task_runner,
                      std::vector<int>* out_run_order)
      : index_(index),
        task_runner_(task_runner),
        out_run_order_(out_run_order) {}

  ~DelayedRunOrderTask() override {}

  void run() override {
    out_run_order_->push_back(index_);
    task_runner_->postTask(
        BLINK_FROM_HERE, new RunOrderTask(index_ + 1, out_run_order_));
  }

 private:
  int index_;
  blink::WebTaskRunner* task_runner_;  // NOT OWNED
  std::vector<int>* out_run_order_;    // NOT OWNED
};
}

TEST_F(WebViewSchedulerImplTest, VirtualTime_DelayedAndImmediateTaskOrdering) {
  std::vector<int> run_order;

  web_view_scheduler_->setAllowVirtualTimeToAdvance(false);
  web_view_scheduler_->enableVirtualTime();

  web_frame_scheduler_->timerTaskRunner()->postTask(
      BLINK_FROM_HERE, new RunOrderTask(0, &run_order));

  web_frame_scheduler_->timerTaskRunner()->postDelayedTask(
      BLINK_FROM_HERE,
      new DelayedRunOrderTask(1, web_frame_scheduler_->timerTaskRunner(),
                              &run_order),
      2.0);

  web_frame_scheduler_->timerTaskRunner()->postDelayedTask(
      BLINK_FROM_HERE,
      new DelayedRunOrderTask(3, web_frame_scheduler_->timerTaskRunner(),
                              &run_order),
      4.0);

  mock_task_runner_->RunUntilIdle();
  EXPECT_TRUE(run_order.empty());

  web_view_scheduler_->setAllowVirtualTimeToAdvance(true);
  mock_task_runner_->RunUntilIdle();

  EXPECT_THAT(run_order, ElementsAre(0, 1, 2, 3, 4));
}

class WebViewSchedulerImplTestWithDisabledBackgroundTimerThrottling
    : public WebViewSchedulerImplTest {
 public:
  WebViewSchedulerImplTestWithDisabledBackgroundTimerThrottling() {}
  ~WebViewSchedulerImplTestWithDisabledBackgroundTimerThrottling() override {}

  bool DisableBackgroundTimerThrottling() const override { return true; }
};

TEST_F(WebViewSchedulerImplTestWithDisabledBackgroundTimerThrottling,
       RepeatingTimer_PageInBackground) {
  web_view_scheduler_->setPageVisible(false);

  int run_count = 0;
  web_frame_scheduler_->timerTaskRunner()->postDelayedTask(
      BLINK_FROM_HERE,
      new RepeatingTask(web_frame_scheduler_->timerTaskRunner(), &run_count),
      1.0);

  mock_task_runner_->RunForPeriod(base::TimeDelta::FromSeconds(1));
  EXPECT_EQ(1000, run_count);
}

TEST_F(WebViewSchedulerImplTest, VirtualTimeSettings_NewWebFrameScheduler) {
  std::vector<int> run_order;

  web_view_scheduler_->setAllowVirtualTimeToAdvance(false);
  web_view_scheduler_->enableVirtualTime();

  scoped_ptr<WebFrameSchedulerImpl> web_frame_scheduler =
      web_view_scheduler_->createWebFrameSchedulerImpl();

  web_frame_scheduler->timerTaskRunner()->postTask(
      BLINK_FROM_HERE, new RunOrderTask(1, &run_order));

  mock_task_runner_->RunUntilIdle();
  EXPECT_TRUE(run_order.empty());

  web_view_scheduler_->setAllowVirtualTimeToAdvance(true);
  mock_task_runner_->RunUntilIdle();

  EXPECT_THAT(run_order, ElementsAre(1));
}

}  // namespace scheduler
