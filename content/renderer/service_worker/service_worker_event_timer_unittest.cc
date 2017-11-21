// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/service_worker/service_worker_event_timer.h"

#include "base/memory/ref_counted.h"
#include "base/message_loop/message_loop.h"
#include "base/test/test_mock_time_task_runner.h"
#include "base/time/tick_clock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {

namespace {

class MockEvent {
 public:
  base::OnceCallback<void(int)> CreateAbortCallback() {
    EXPECT_FALSE(has_aborted_);
    return base::BindOnce(&MockEvent::Abort, base::Unretained(this));
  }

  int event_id() const { return event_id_; }
  void set_event_id(int event_id) { event_id_ = event_id; }
  bool has_aborted() const { return has_aborted_; }

 private:
  void Abort(int event_id) {
    EXPECT_EQ(event_id_, event_id);
    has_aborted_ = true;
  }

  bool has_aborted_ = false;
  int event_id_ = 0;
};

}  // namespace

class ServiceWorkerEventTimerTest : public testing::Test {
 protected:
  void SetUp() override {
    task_runner_ = base::MakeRefCounted<base::TestMockTimeTaskRunner>(
        base::Time::Now(), base::TimeTicks::Now());
    message_loop_.SetTaskRunner(task_runner_);
  }

  base::TestMockTimeTaskRunner* task_runner() { return task_runner_.get(); }

 private:
  base::MessageLoop message_loop_;
  scoped_refptr<base::TestMockTimeTaskRunner> task_runner_;
};

TEST_F(ServiceWorkerEventTimerTest, IdleTimer) {
  const base::TimeDelta kIdleInterval =
      ServiceWorkerEventTimer::kIdleDelay +
      ServiceWorkerEventTimer::kUpdateInterval +
      base::TimeDelta::FromSeconds(1);

  base::RepeatingCallback<void(int)> do_nothing_callback =
      base::BindRepeating([](int) {});

  bool is_idle = false;
  ServiceWorkerEventTimer timer(
      base::BindRepeating([](bool* out_is_idle) { *out_is_idle = true; },
                          &is_idle),
      task_runner()->GetMockTickClock());
  task_runner()->FastForwardBy(kIdleInterval);
  // |idle_callback| should be fired since there is no event.
  EXPECT_TRUE(is_idle);

  is_idle = false;
  int event_id_1 = timer.StartEvent(do_nothing_callback);
  task_runner()->FastForwardBy(kIdleInterval);
  // Nothing happens since there is an inflight event.
  EXPECT_FALSE(is_idle);

  int event_id_2 = timer.StartEvent(do_nothing_callback);
  task_runner()->FastForwardBy(kIdleInterval);
  // Nothing happens since there are two inflight events.
  EXPECT_FALSE(is_idle);

  timer.EndEvent(event_id_2);
  task_runner()->FastForwardBy(kIdleInterval);
  // Nothing happens since there is an inflight event.
  EXPECT_FALSE(is_idle);

  timer.EndEvent(event_id_1);
  task_runner()->FastForwardBy(kIdleInterval);
  // |idle_callback| should be fired.
  EXPECT_TRUE(is_idle);
}

TEST_F(ServiceWorkerEventTimerTest, EventTimer) {
  ServiceWorkerEventTimer timer(base::BindRepeating(&base::DoNothing),
                                task_runner()->GetMockTickClock());
  MockEvent event1, event2;

  int event_id1 = timer.StartEvent(event1.CreateAbortCallback());
  int event_id2 = timer.StartEvent(event2.CreateAbortCallback());
  event1.set_event_id(event_id1);
  event2.set_event_id(event_id2);
  task_runner()->FastForwardBy(ServiceWorkerEventTimer::kUpdateInterval +
                               base::TimeDelta::FromSeconds(1));

  EXPECT_FALSE(event1.has_aborted());
  EXPECT_FALSE(event2.has_aborted());
  timer.EndEvent(event1.event_id());
  task_runner()->FastForwardBy(ServiceWorkerEventTimer::kEventTimeout +
                               base::TimeDelta::FromSeconds(1));

  EXPECT_FALSE(event1.has_aborted());
  EXPECT_TRUE(event2.has_aborted());
}

}  // namespace content
