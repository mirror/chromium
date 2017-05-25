// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/bindings/RuntimeCallStats.h"

#include "platform/wtf/CurrentTime.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace blink {

namespace {

unsigned ticks_ = 1;

void Sleep(double milliseconds) {
  ticks_ += milliseconds;
}

}  // namespace

class RuntimeCallStatsTest : public testing::Test {
 public:
  void SetUp() override {
    original_time_function_ = SetTimeFunctionsForTesting([] {
      return static_cast<double>(ticks_) /
             static_cast<double>(base::Time::kMillisecondsPerSecond);
    });
  }

  void TearDown() override {
    SetTimeFunctionsForTesting(original_time_function_);
  }

 private:
  TimeFunction original_time_function_;
};

TEST_F(RuntimeCallStatsTest, TimeTicksSanityCheck) {
  TimeTicks start = TimeTicks::Now();
  Sleep(50);
  EXPECT_EQ(50, (TimeTicks::Now() - start).InMilliseconds());
}

TEST_F(RuntimeCallStatsTest, InitialCountShouldBeZero) {
  RuntimeCallCounter counter("counter");
  EXPECT_EQ(0ul, counter.GetCount());
}

TEST_F(RuntimeCallStatsTest, CountIsUpdatedAfterLeave) {
  RuntimeCallCounter counter("counter");
  RuntimeCallTimer timer;
  RuntimeCallStats stats;

  RuntimeCallStats::Enter(&stats, &timer, &counter);
  EXPECT_EQ(0ul, counter.GetCount());
  RuntimeCallStats::Leave(&stats, &timer);
  EXPECT_EQ(1ul, counter.GetCount());
}

TEST_F(RuntimeCallStatsTest, TimeIsUpdatedAfterLeave) {
  RuntimeCallCounter counter("counter");
  RuntimeCallTimer timer;
  RuntimeCallStats stats;

  RuntimeCallStats::Enter(&stats, &timer, &counter);
  Sleep(50);
  RuntimeCallStats::Leave(&stats, &timer);
  EXPECT_EQ(50, counter.GetTime().InMilliseconds());
}

TEST_F(RuntimeCallStatsTest, CountAndTimeAreUpdatedAfterMultipleExecutions) {
  RuntimeCallCounter counter("counter");
  RuntimeCallStats stats;

  const unsigned func_duration = 20;
  const unsigned loops = 5;

  auto func = [&counter, &stats, func_duration]() {
    RuntimeCallTimer timer;
    RuntimeCallStats::Enter(&stats, &timer, &counter);
    Sleep(func_duration);
    RuntimeCallStats::Leave(&stats, &timer);
  };

  for (unsigned i = 0; i < loops; i++)
    func();

  EXPECT_EQ((uint64_t)loops, counter.GetCount());
  EXPECT_EQ(loops * func_duration, counter.GetTime().InMilliseconds());
}

TEST_F(RuntimeCallStatsTest, NestedTimersTest) {
  RuntimeCallStats stats;
  RuntimeCallCounter outer_counter("outer_counter");
  RuntimeCallCounter inner_counter("inner_counter");

  const unsigned inner_func_duration = 50;
  const unsigned outer_func_duration = 20;

  auto inner_func = [&stats, &inner_counter, inner_func_duration]() {
    RuntimeCallTimer timer;
    RuntimeCallStats::Enter(&stats, &timer, &inner_counter);
    Sleep(inner_func_duration);
    RuntimeCallStats::Leave(&stats, &timer);
  };

  auto outer_func = [&stats, &outer_counter, &inner_func,
                     outer_func_duration]() {
    RuntimeCallTimer timer;
    RuntimeCallStats::Enter(&stats, &timer, &outer_counter);
    inner_func();
    Sleep(outer_func_duration);
    RuntimeCallStats::Leave(&stats, &timer);
  };

  outer_func();

  EXPECT_EQ(1ul, outer_counter.GetCount());
  EXPECT_EQ(1ul, inner_counter.GetCount());
  EXPECT_EQ(outer_func_duration, outer_counter.GetTime().InMilliseconds());
  EXPECT_EQ(inner_func_duration, inner_counter.GetTime().InMilliseconds());
}

TEST_F(RuntimeCallStatsTest, RuntimeCallTimerScopeTest) {
  RuntimeCallStats stats;
  RuntimeCallCounter counter("counter");

  auto func = [&stats, &counter]() {
    RuntimeCallTimerScope scope(&stats, &counter);
    Sleep(50);
  };

  func();

  EXPECT_EQ(1ul, counter.GetCount());
  EXPECT_EQ(50, counter.GetTime().InMilliseconds());

  func();

  EXPECT_EQ(2ul, counter.GetCount());
  EXPECT_EQ(100, counter.GetTime().InMilliseconds());
}

TEST_F(RuntimeCallStatsTest, RecursiveFunctionWithScopeTest) {
  RuntimeCallStats stats;
  RuntimeCallCounter counter("counter");

  std::function<void(int)> recursive_func;
  recursive_func = [&stats, &counter, &recursive_func](int x) {
    RuntimeCallTimerScope scope(&stats, &counter);
    if (x <= 0)
      return;
    Sleep(50);
    recursive_func(x - 1);
  };
  recursive_func(5);

  EXPECT_EQ(6ul, counter.GetCount());
  EXPECT_EQ(250, counter.GetTime().InMilliseconds());
}

TEST_F(RuntimeCallStatsTest, ReuseTimer) {
  RuntimeCallStats stats;
  RuntimeCallTimer timer;
  RuntimeCallCounter counter1("counter1");
  RuntimeCallCounter counter2("counter2");

  RuntimeCallStats::Enter(&stats, &timer, &counter1);
  Sleep(50);
  RuntimeCallStats::Leave(&stats, &timer);

  timer.Reset();

  RuntimeCallStats::Enter(&stats, &timer, &counter2);
  Sleep(25);
  RuntimeCallStats::Leave(&stats, &timer);

  EXPECT_EQ(1ul, counter1.GetCount());
  EXPECT_EQ(1ul, counter2.GetCount());
  EXPECT_EQ(50, counter1.GetTime().InMilliseconds());
  EXPECT_EQ(25, counter2.GetTime().InMilliseconds());
}

}  // namespace blink
