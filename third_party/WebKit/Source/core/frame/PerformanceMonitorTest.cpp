// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/frame/PerformanceMonitor.h"

#include "core/frame/LocalFrame.h"
#include "core/frame/Location.h"
#include "core/probe/CoreProbes.h"
#include "core/testing/DummyPageHolder.h"
#include "platform/wtf/CurrentTime.h"
#include "platform/wtf/PtrUtil.h"
#include "testing/gtest/include/gtest/gtest.h"

#include <memory>

namespace blink {

static double g_mock_time = 1000.;

class FakeTimer {
 public:
  FakeTimer(double time_stamp) {
    g_mock_time = time_stamp;
    original_time_function_ =
        WTF::SetTimeFunctionsForTesting(returnMockTimeInSeconds);
  }

  ~FakeTimer() { WTF::SetTimeFunctionsForTesting(original_time_function_); }

  double GetMockTimeInSeconds() { return g_mock_time; }

  void SetMockTimeInSeconds(double mock_time) { g_mock_time = mock_time; }

  void IncrementDuration(double duration_in_seconds) {
    g_mock_time += duration_in_seconds;
  }

 private:
  static double returnMockTimeInSeconds() { return g_mock_time; }
  TimeFunction original_time_function_;
};

class PerformanceMonitorTest : public ::testing::Test {
 protected:
  void SetUp() override;
  void TearDown() override;
  LocalFrame* GetFrame() const {
    return page_holder_->GetDocument().GetFrame();
  }
  ExecutionContext* GetExecutionContext() const {
    return &page_holder_->GetDocument();
  }
  LocalFrame* AnotherFrame() const {
    return another_page_holder_->GetDocument().GetFrame();
  }
  ExecutionContext* AnotherExecutionContext() const {
    return &another_page_holder_->GetDocument();
  }

  void WillExecuteScript(ExecutionContext* execution_context) {
    monitor_->WillExecuteScript(execution_context);
  }

  // scheduler::TaskTimeObserver implementation
  void WillProcessTask(double start_time) {
    monitor_->WillProcessTask(start_time);
  }

  void DidProcessTask(double start_time, double end_time) {
    monitor_->DidProcessTask(start_time, end_time);
  }

  void Will(probe::V8Compile& probe) { monitor_->Will(probe); }

  void Did(probe::V8Compile& probe) { monitor_->Did(probe); }

  void SubscribeLongTask() {
    monitor_->enabled_ = true;
    monitor_->thresholds_[PerformanceMonitor::kLongTask] = true;
  }

  SubTaskAttribution::EntriesVector& SubTaskAttributions() {
    return monitor_->sub_task_attributions_;
  }

  void SimulateV8CompileTask(FakeTimer& timer,
                             double duration,
                             String& script_url,
                             int line,
                             int column) {
    probe::V8Compile probe(GetExecutionContext(), script_url, line, column);
    timer.IncrementDuration(duration);
  }

  String FrameContextURL();
  int NumUniqueFrameContextsSeen();

  Persistent<PerformanceMonitor> monitor_;
  std::unique_ptr<DummyPageHolder> page_holder_;
  std::unique_ptr<DummyPageHolder> another_page_holder_;
};

void PerformanceMonitorTest::SetUp() {
  page_holder_ = DummyPageHolder::Create(IntSize(800, 600));
  page_holder_->GetDocument().SetURL(
      KURL(NullURL(), "https://example.com/foo"));
  monitor_ = new PerformanceMonitor(GetFrame());

  // Create another dummy page holder and pretend this is the iframe.
  another_page_holder_ = DummyPageHolder::Create(IntSize(400, 300));
  another_page_holder_->GetDocument().SetURL(
      KURL(NullURL(), "https://iframed.com/bar"));
}

void PerformanceMonitorTest::TearDown() {
  monitor_->Shutdown();
}

String PerformanceMonitorTest::FrameContextURL() {
  // This is reported only if there is a single frameContext URL.
  if (monitor_->task_has_multiple_contexts_)
    return "";
  Frame* frame = ToDocument(monitor_->task_execution_context_)->GetFrame();
  return ToLocalFrame(frame)->GetDocument()->location()->href();
}

int PerformanceMonitorTest::NumUniqueFrameContextsSeen() {
  if (!monitor_->task_execution_context_)
    return 0;
  if (!monitor_->task_has_multiple_contexts_)
    return 1;
  return 2;
}

TEST_F(PerformanceMonitorTest, SingleScriptInTask) {
  WillProcessTask(3719349.445172);
  EXPECT_EQ(0, NumUniqueFrameContextsSeen());
  WillExecuteScript(GetExecutionContext());
  EXPECT_EQ(1, NumUniqueFrameContextsSeen());
  DidProcessTask(3719349.445172, 3719349.5561923);  // Long task
  EXPECT_EQ(1, NumUniqueFrameContextsSeen());
  EXPECT_EQ("https://example.com/foo", FrameContextURL());
}

TEST_F(PerformanceMonitorTest, MultipleScriptsInTask_SingleContext) {
  WillProcessTask(3719349.445172);
  EXPECT_EQ(0, NumUniqueFrameContextsSeen());
  WillExecuteScript(GetExecutionContext());
  EXPECT_EQ(1, NumUniqueFrameContextsSeen());
  EXPECT_EQ("https://example.com/foo", FrameContextURL());

  WillExecuteScript(GetExecutionContext());
  EXPECT_EQ(1, NumUniqueFrameContextsSeen());
  DidProcessTask(3719349.445172, 3719349.5561923);  // Long task
  EXPECT_EQ(1, NumUniqueFrameContextsSeen());
  EXPECT_EQ("https://example.com/foo", FrameContextURL());
}

TEST_F(PerformanceMonitorTest, MultipleScriptsInTask_MultipleContexts) {
  WillProcessTask(3719349.445172);
  EXPECT_EQ(0, NumUniqueFrameContextsSeen());
  WillExecuteScript(GetExecutionContext());
  EXPECT_EQ(1, NumUniqueFrameContextsSeen());
  EXPECT_EQ("https://example.com/foo", FrameContextURL());

  WillExecuteScript(AnotherExecutionContext());
  EXPECT_EQ(2, NumUniqueFrameContextsSeen());
  DidProcessTask(3719349.445172, 3719349.5561923);  // Long task
  EXPECT_EQ(2, NumUniqueFrameContextsSeen());
  EXPECT_EQ("", FrameContextURL());
}

TEST_F(PerformanceMonitorTest, NoScriptInLongTask) {
  WillProcessTask(3719349.445172);
  WillExecuteScript(GetExecutionContext());
  DidProcessTask(3719349.445172, 3719349.445182);

  WillProcessTask(3719349.445172);
  DidProcessTask(3719349.445172, 3719349.5561923);  // Long task
  // Without presence of Script, FrameContext URL is not available
  EXPECT_EQ(0, NumUniqueFrameContextsSeen());
}

TEST_F(PerformanceMonitorTest, SubTaskAttribution_V8Compile) {
  FakeTimer timer(1000.);
  String script_url = "http://abc.def";
  int line = 1;
  int column = 2;
  SubscribeLongTask();

  WillProcessTask(0);

  SimulateV8CompileTask(timer, 0.001, script_url, 0, 0);
  EXPECT_EQ(0, (int)SubTaskAttributions().size());

  SimulateV8CompileTask(timer, 0.013, script_url, line, column);
  EXPECT_EQ(1, (int)SubTaskAttributions().size());
  EXPECT_FLOAT_EQ(1000.001, SubTaskAttributions()[0]->startTime());
  EXPECT_FLOAT_EQ(0.013, SubTaskAttributions()[0]->duration());
  EXPECT_EQ("script-compile", SubTaskAttributions()[0]->subTaskName());
  EXPECT_EQ(
      String::Format("%s(%d, %d)", script_url.Utf8().data(), line, column),
      SubTaskAttributions()[0]->scriptURL());
}

}  // namespace blink
