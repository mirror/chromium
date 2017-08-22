// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/timing/Performance.h"

#include "bindings/core/v8/PerformanceObserverCallback.h"
#include "bindings/core/v8/V8BindingForTesting.h"
#include "core/frame/PerformanceMonitor.h"
#include "core/loader/DocumentLoadTiming.h"
#include "core/loader/DocumentLoader.h"
#include "core/probe/CoreProbes.h"
#include "core/testing/DummyPageHolder.h"
#include "core/timing/DOMWindowPerformance.h"
#include "core/timing/PerformanceLongTaskTiming.h"
#include "core/timing/PerformanceObserver.h"
#include "core/timing/PerformanceObserverInit.h"
#include "core/timing/TaskAttributionTiming.h"
#include "platform/RuntimeEnabledFeatures.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace blink {
using PerformanceEntryVector = HeapVector<Member<PerformanceEntry>>;
using PerformanceObservers = HeapListHashSet<Member<PerformanceObserver>>;

namespace {
class FakeTimer {
 public:
  FakeTimer() {
    g_mock_time = 1000.;
    original_time_function_ =
        WTF::SetTimeFunctionsForTesting(GetMockTimeInSeconds);
  }

  ~FakeTimer() { WTF::SetTimeFunctionsForTesting(original_time_function_); }

  static double GetMockTimeInSeconds() { return g_mock_time; }

  void ForwardTimer(double mock_time) { g_mock_time += mock_time; }

 private:
  TimeFunction original_time_function_;
  static double g_mock_time;
};

double FakeTimer::g_mock_time = 1000.;
}  // namespace

class PerformanceTest : public ::testing::Test {
 protected:
  void SetUp() override {
    V8TestingScope scope;
    page_holder_ = DummyPageHolder::Create(IntSize(800, 600));
    page_holder_->GetDocument().SetURL(KURL(NullURL(), "https://example.com"));
    performance_ = Performance::Create(&page_holder_->GetFrame());
    v8::Local<v8::Function> callback =
        v8::Function::New(scope.GetScriptState()->GetContext(), nullptr)
            .ToLocalChecked();
    call_bask_ =
        PerformanceObserverCallback::Create(scope.GetScriptState(), callback);
    observer_ = new PerformanceObserver(performance_->GetExecutionContext(),
                                        performance_, call_bask_);

    // Create another dummy page holder and pretend this is the iframe.
    another_page_holder_ = DummyPageHolder::Create(IntSize(400, 300));
    another_page_holder_->GetDocument().SetURL(
        KURL(NullURL(), "https://iframed.com/bar"));
  }

  bool ObservingLongTasks() {
    return PerformanceMonitor::InstrumentingMonitor(
        performance_->GetExecutionContext());
  }

  void AddLongTaskObserver() {
    // simulate with filter options.
    performance_->observer_filter_options_ |= PerformanceEntry::kLongTask;
  }

  void RemoveLongTaskObserver() {
    // simulate with filter options.
    performance_->observer_filter_options_ = PerformanceEntry::kInvalid;
  }

  void SimulateDidProcessLongTask() {
    auto* monitor = GetFrame()->GetPerformanceMonitor();
    AddTaskExecutionContext();
    monitor->DidProcessTask(0, 1);
  }

  void AddTaskExecutionContext() {
    auto* monitor = GetFrame()->GetPerformanceMonitor();
    monitor->WillExecuteScript(GetDocument());
    monitor->DidExecuteScript();
  }

  void WillProcessTask(double start_time) {
    GetFrame()->GetPerformanceMonitor()->WillProcessTask(start_time);
  }

  void DidProcessTask(double start_time, double end_time) {
    GetFrame()->GetPerformanceMonitor()->DidProcessTask(start_time, end_time);
  }

  void SimulateV8CompileTask(FakeTimer& timer,
                             double duration,
                             String& script_url,
                             int line,
                             int column) {
    probe::V8Compile probe(performance_->GetExecutionContext(), script_url,
                           line, column);
    timer.ForwardTimer(duration);
  }

  PerformanceObservers& GetPerformanceObservers() {
    return performance_->observers_;
  }

  PerformanceEntryVector& GetPerformanceEntries(
      const Member<PerformanceObserver>& observer) {
    return observer->performance_entries_;
  }

  LocalFrame* GetFrame() const { return &page_holder_->GetFrame(); }

  Document* GetDocument() const { return &page_holder_->GetDocument(); }

  LocalFrame* AnotherFrame() const { return &another_page_holder_->GetFrame(); }

  Document* AnotherDocument() const {
    return &another_page_holder_->GetDocument();
  }

  String SanitizedAttribution(ExecutionContext* context,
                              bool has_multiple_contexts,
                              LocalFrame* observer_frame) {
    return Performance::SanitizedAttribution(context, has_multiple_contexts,
                                             observer_frame)
        .first;
  }

  Persistent<Performance> performance_;
  Persistent<PerformanceObserver> observer_;
  Persistent<PerformanceObserverCallback> call_bask_;
  std::unique_ptr<DummyPageHolder> page_holder_;
  std::unique_ptr<DummyPageHolder> another_page_holder_;
};

TEST_F(PerformanceTest, LongTaskObserverInstrumentation) {
  performance_->UpdateLongTaskInstrumentation();
  EXPECT_FALSE(ObservingLongTasks());

  // Adding LongTask observer (with filer option) enables instrumentation.
  AddLongTaskObserver();
  performance_->UpdateLongTaskInstrumentation();
  EXPECT_TRUE(ObservingLongTasks());

  // Removing LongTask observer disables instrumentation.
  RemoveLongTaskObserver();
  performance_->UpdateLongTaskInstrumentation();
  EXPECT_FALSE(ObservingLongTasks());
}

TEST_F(PerformanceTest, SanitizedLongTaskName) {
  // Unable to attribute, when no execution contents are available.
  EXPECT_EQ("unknown", SanitizedAttribution(nullptr, false, GetFrame()));

  // Attribute for same context (and same origin).
  EXPECT_EQ("self", SanitizedAttribution(GetDocument(), false, GetFrame()));

  // Unable to attribute, when multiple script execution contents are involved.
  EXPECT_EQ("multiple-contexts",
            SanitizedAttribution(GetDocument(), true, GetFrame()));
}

TEST_F(PerformanceTest, SanitizedLongTaskName_CrossOrigin) {
  // Unable to attribute, when no execution contents are available.
  EXPECT_EQ("unknown", SanitizedAttribution(nullptr, false, GetFrame()));

  // Attribute for same context (and same origin).
  EXPECT_EQ("cross-origin-unreachable",
            SanitizedAttribution(AnotherDocument(), false, GetFrame()));
}

// https://crbug.com/706798: Checks that after navigation that have replaced the
// window object, calls to not garbage collected yet Performance belonging to
// the old window do not cause a crash.
TEST_F(PerformanceTest, NavigateAway) {
  AddLongTaskObserver();
  performance_->UpdateLongTaskInstrumentation();
  EXPECT_TRUE(ObservingLongTasks());

  // Simulate navigation commit.
  DocumentInit init = DocumentInit::Create().WithFrame(GetFrame());
  GetDocument()->Shutdown();
  GetFrame()->SetDOMWindow(LocalDOMWindow::Create(*GetFrame()));
  GetFrame()->DomWindow()->InstallNewDocument(AtomicString(), init, false);

  // m_performance is still alive, and should not crash when notified.
  SimulateDidProcessLongTask();
}

// Checks that Performance object and its fields (like PerformanceTiming)
// function correctly after transition to another document in the same window.
// This happens when a page opens a new window and it navigates to a same-origin
// document.
TEST(PerformanceLifetimeTest, SurviveContextSwitch) {
  std::unique_ptr<DummyPageHolder> page_holder =
      DummyPageHolder::Create(IntSize(800, 600));

  Performance* perf =
      DOMWindowPerformance::performance(*page_holder->GetFrame().DomWindow());
  PerformanceTiming* timing = perf->timing();

  auto* document_loader = page_holder->GetFrame().Loader().GetDocumentLoader();
  ASSERT_TRUE(document_loader);
  document_loader->GetTiming().SetNavigationStart(
      MonotonicallyIncreasingTime());

  EXPECT_EQ(&page_holder->GetFrame(), perf->GetFrame());
  EXPECT_EQ(&page_holder->GetFrame(), timing->GetFrame());
  auto navigation_start = timing->navigationStart();
  EXPECT_NE(0U, navigation_start);

  // Simulate changing the document while keeping the window.
  page_holder->GetDocument().Shutdown();
  page_holder->GetFrame().DomWindow()->InstallNewDocument(
      AtomicString(),
      DocumentInit::Create().WithFrame(&page_holder->GetFrame()), false);

  EXPECT_EQ(perf, DOMWindowPerformance::performance(
                      *page_holder->GetFrame().DomWindow()));
  EXPECT_EQ(timing, perf->timing());
  EXPECT_EQ(&page_holder->GetFrame(), perf->GetFrame());
  EXPECT_EQ(&page_holder->GetFrame(), timing->GetFrame());
  EXPECT_EQ(navigation_start, timing->navigationStart());
}

TEST_F(PerformanceTest, LongTaskV1_SubTaskAttribution) {
  RuntimeEnabledFeatures::SetLongTaskV2Enabled(false);
  FakeTimer timer;
  String script_url = "http://abc.def";
  int line = 1;
  int column = 2;

  AddLongTaskObserver();

  // Make an observer for longtask
  NonThrowableExceptionState exception_state;
  PerformanceObserverInit options;
  Vector<String> entry_type_vec;
  entry_type_vec.push_back("longtask");
  options.setEntryTypes(entry_type_vec);
  observer_->observe(options, exception_state);

  performance_->RegisterPerformanceObserver(*observer_);

  double start_time = timer.GetMockTimeInSeconds();
  WillProcessTask(start_time);
  AddTaskExecutionContext();

  // Too short to be considered as a long subtask.
  SimulateV8CompileTask(timer, 0.001, script_url, 0, 0);
  EXPECT_FLOAT_EQ(1000.001, timer.GetMockTimeInSeconds());
  // Simulate a long subtask.
  SimulateV8CompileTask(timer, 0.013, script_url, line, column);
  EXPECT_FLOAT_EQ(1000.014, timer.GetMockTimeInSeconds());

  timer.ForwardTimer(0.050);
  EXPECT_FLOAT_EQ(1000.064, timer.GetMockTimeInSeconds());
  DidProcessTask(start_time, timer.GetMockTimeInSeconds());
  PerformanceObservers& observers = GetPerformanceObservers();
  EXPECT_EQ(1, (int)observers.size());

  for (auto& observer : observers) {
    PerformanceEntryVector& entries = GetPerformanceEntries(observer);
    EXPECT_EQ(1, (int)entries.size());
    Member<PerformanceEntry>& entry = entries[0];

    // Start time and duration being 0 is because the time_origin of
    // PerformanceBase is set to 0.
    EXPECT_FLOAT_EQ(0, entry->startTime());
    EXPECT_FLOAT_EQ(0, entry->duration());
    EXPECT_EQ("self", entry->name());
    EXPECT_EQ("longtask", entry->entryType());
    PerformanceEntry* entry_ptr = entry.Release();
    PerformanceLongTaskTiming* long_task_entry =
        (PerformanceLongTaskTiming*)entry_ptr;
    TaskAttributionVector sub_task_attributions =
        long_task_entry->attribution();
    EXPECT_EQ(1, (int)sub_task_attributions.size());
    Member<TaskAttributionTiming> sub_task_attribution =
        sub_task_attributions[0];
    EXPECT_EQ("", sub_task_attribution->scriptURL());
    EXPECT_EQ("iframe", sub_task_attribution->containerType());
    EXPECT_EQ("", sub_task_attribution->containerSrc());
    EXPECT_EQ("", sub_task_attribution->containerId());
    EXPECT_EQ("", sub_task_attribution->containerName());
    EXPECT_EQ("script", sub_task_attribution->name());
    EXPECT_EQ("taskattribution", sub_task_attribution->entryType());
    EXPECT_FLOAT_EQ(0, sub_task_attribution->startTime());
    EXPECT_FLOAT_EQ(0, sub_task_attribution->duration());
  }

  RemoveLongTaskObserver();
}

TEST_F(PerformanceTest, LongTaskV2_SubTaskAttribution) {
  RuntimeEnabledFeatures::SetLongTaskV2Enabled(true);
  FakeTimer timer;
  String script_url = "http://abc.def";
  int line = 1;
  int column = 2;

  AddLongTaskObserver();

  // Make an observer for longtask
  NonThrowableExceptionState exception_state;
  PerformanceObserverInit options;
  Vector<String> entry_type_vec;
  entry_type_vec.push_back("longtask");
  options.setEntryTypes(entry_type_vec);
  observer_->observe(options, exception_state);

  performance_->RegisterPerformanceObserver(*observer_);

  double start_time = timer.GetMockTimeInSeconds();
  WillProcessTask(start_time);
  AddTaskExecutionContext();
  // Too short to be considered as a long subtask.
  SimulateV8CompileTask(timer, 0.001, script_url, 0, 0);
  EXPECT_FLOAT_EQ(1000.001, timer.GetMockTimeInSeconds());
  // Simulate a long subtask.
  SimulateV8CompileTask(timer, 0.013, script_url, line, column);
  EXPECT_FLOAT_EQ(1000.014, timer.GetMockTimeInSeconds());

  timer.ForwardTimer(0.050);
  EXPECT_FLOAT_EQ(1000.064, timer.GetMockTimeInSeconds());
  DidProcessTask(start_time, timer.GetMockTimeInSeconds());
  PerformanceObservers& observers = GetPerformanceObservers();
  EXPECT_EQ(1, (int)observers.size());

  for (auto& observer : observers) {
    PerformanceEntryVector& entries = GetPerformanceEntries(observer);
    EXPECT_EQ(1, (int)entries.size());
    Member<PerformanceEntry>& entry = entries[0];

    // Start time and duration being 0 is because the time_origin of
    // PerformanceBase is set to 0.
    EXPECT_FLOAT_EQ(0, entry->startTime());
    EXPECT_FLOAT_EQ(0, entry->duration());
    EXPECT_EQ("self", entry->name());
    EXPECT_EQ("longtask", entry->entryType());
    PerformanceEntry* entry_ptr = entry.Release();
    PerformanceLongTaskTiming* long_task_entry =
        (PerformanceLongTaskTiming*)entry_ptr;
    TaskAttributionVector sub_task_attributions =
        long_task_entry->attribution();
    EXPECT_EQ(1, (int)sub_task_attributions.size());
    Member<TaskAttributionTiming> sub_task_attribution =
        sub_task_attributions[0];
    EXPECT_EQ("http://abc.def(1, 2)", sub_task_attribution->scriptURL());
    EXPECT_EQ("iframe", sub_task_attribution->containerType());
    EXPECT_EQ("", sub_task_attribution->containerSrc());
    EXPECT_EQ("", sub_task_attribution->containerId());
    EXPECT_EQ("", sub_task_attribution->containerName());
    EXPECT_EQ("script-compile", sub_task_attribution->name());
    EXPECT_EQ("taskattribution", sub_task_attribution->entryType());
    EXPECT_FLOAT_EQ(0, sub_task_attribution->startTime());
    EXPECT_FLOAT_EQ(13, sub_task_attribution->duration());
  }

  RemoveLongTaskObserver();
}
}  // namespace blink
