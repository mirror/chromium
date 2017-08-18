// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/timing/Performance.h"

#include "core/frame/PerformanceMonitor.h"
#include "core/loader/DocumentLoadTiming.h"
#include "core/loader/DocumentLoader.h"
#include "core/probe/CoreProbes.h"
#include "core/testing/DummyPageHolder.h"
#include "core/timing/DOMWindowPerformance.h"
#include "core/timing/PerformanceObserver.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace blink {
using PerformanceEntryVector = HeapVector<Member<PerformanceEntry>>;
using PerformanceObservers = HeapListHashSet<Member<PerformanceObserver>>;

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

class PerformanceTest : public ::testing::Test {
 protected:
  void SetUp() override {
    page_holder_ = DummyPageHolder::Create(IntSize(800, 600));
    page_holder_->GetDocument().SetURL(KURL(NullURL(), "https://example.com"));
    performance_ = Performance::Create(&page_holder_->GetFrame());
    observer_ = new PerformanceObserver(performance_->GetExecutionContext(),
                                        performance_, nullptr);

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
    monitor->WillExecuteScript(GetDocument());
    monitor->DidExecuteScript();
    monitor->DidProcessTask(0, 1);
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
    timer.IncrementDuration(duration);
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

TEST_F(PerformanceTest, SubTaskAttribution) {
  FakeTimer timer(1000.);
  String script_url = "http://abc.def";
  int line = 1;
  int column = 2;

  AddLongTaskObserver();
  performance_->RegisterPerformanceObserver(*observer_);

  double start_time = timer.GetMockTimeInSeconds();
  WillProcessTask(start_time);
  // Too short to be considered as a long subtask.
  SimulateV8CompileTask(timer, 0.001, script_url, 0, 0);
  // Simulate a long subtask.
  SimulateV8CompileTask(timer, 0.013, script_url, line, column);

  timer.IncrementDuration(1);
  printf("!perf->GetFrame()=%d\n", !performance_->GetFrame());
  DidProcessTask(start_time, timer.GetMockTimeInSeconds());
  printf("!perf->GetFrame()=%d\n", !performance_->GetFrame());
  auto& observers = GetPerformanceObservers();
  EXPECT_EQ(1, (int)observers.size());
  for (const auto& observer : GetPerformanceObservers()) {
    PerformanceEntryVector& entries = GetPerformanceEntries(observer);
    EXPECT_EQ(1, (int)entries.size());
    auto& entry = entries[0];
    EXPECT_FLOAT_EQ(1000000, entry->startTime());
    EXPECT_FLOAT_EQ(13, entry->duration());
    EXPECT_EQ("", entry->name());
    EXPECT_EQ("", entry->entryType());
    // entry.sub_task_attributions()
  }
  RemoveLongTaskObserver();
}
}  // namespace blink
