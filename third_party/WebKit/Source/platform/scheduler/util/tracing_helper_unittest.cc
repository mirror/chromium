// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/scheduler/util/tracing_helper.h"

#include <memory>
#include <vector>

#include "testing/gtest/include/gtest/gtest.h"

namespace blink {
namespace scheduler {

namespace {

const char* g_last_state = nullptr;

void ExpectTraced(const char* state) {
  EXPECT_TRUE(state);
  EXPECT_TRUE(g_last_state);
  EXPECT_STREQ(state, g_last_state);
  g_last_state = nullptr;
}

void ExpectNotTraced() {
  EXPECT_FALSE(g_last_state);
}

const char* SignOfInt(int value) {
  if (value > 0) {
    return "positive";
  } else if (value < 0) {
    return "negative";
  } else {
    return "zero";
  }
}

class Controller : public TraceableVariableController {
 public:
  Controller() {}
  virtual ~Controller() {}

  void RegisterTraceableVariable(TraceableVariable* tracer) final {
    tracers_.push_back(tracer);
  }

  void OnTraceLogEnabled() {
    for (auto tracer : tracers_) {
      tracer->OnTraceLogEnabled();
    }
  }

 private:
  std::vector<TraceableVariable*> tracers_;
};

}  // namespace

namespace internal {

class TraceableStateForTest
    : public TraceableState<int, kTracingCategoryNameDefault> {
 public:
  TraceableStateForTest(TraceableVariableController* controller)
      : TraceableState(0, "State", controller, SignOfInt) {
    // We shouldn't expect trace in constructor here because mock isn't set yet.
    mock_trace_for_test_ = &MockTrace;
  }

  TraceableStateForTest& operator =(const int& value) {
    Assign(value);
    return *this;
  }

  static void MockTrace(const char* state) {
    EXPECT_TRUE(state);
    EXPECT_FALSE(g_last_state);  // No unexpected traces.
    g_last_state = state;
  }
};

}  // namespace internal

// TODO(kraynov): TraceableCounter tests.

TEST(TracingHelperTest, TraceableState) {
  std::unique_ptr<Controller> controller(new Controller());
  internal::TraceableStateForTest state(controller.get());
  controller->OnTraceLogEnabled();
  ExpectTraced("zero");
  state = 0;
  ExpectNotTraced();
  state = 1;
  ExpectTraced("positive");
  state = -1;
  ExpectTraced("negative");
}

TEST(TracingHelperTest, TraceableStateOperators) {
  std::unique_ptr<Controller> controller(new Controller());
  TraceableState<int, kTracingCategoryNameDebug> x(
      -1, "X", controller.get(), SignOfInt);
  TraceableState<int, kTracingCategoryNameDebug> y(
      1, "Y", controller.get(), SignOfInt);
  EXPECT_EQ(0, x + y);
  EXPECT_FALSE(x == y);
  EXPECT_TRUE(x != y);
  x = 1;
  EXPECT_EQ(0, y - x);
  EXPECT_EQ(2, x + y);
  EXPECT_TRUE(x == y);
  EXPECT_FALSE(x != y);
  EXPECT_TRUE((x + y) != 3);
  EXPECT_TRUE((2 - y + 1 + x) == 3);
  x = 3;
  y = 2;
  int z = x = y;
  EXPECT_EQ(2, z);
}

}  // namespace scheduler
}  // namespace blink
