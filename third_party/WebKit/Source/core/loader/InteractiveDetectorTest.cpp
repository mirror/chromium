// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/loader/InteractiveDetector.h"

#include "core/dom/Document.h"
#include "core/testing/DummyPageHolder.h"
#include "platform/CrossThreadFunctional.h"
#include "platform/scheduler/renderer/renderer_scheduler_impl.h"
#include "platform/testing/TestingPlatformSupport.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace blink {

class NetworkActivityCheckerForTest
    : public InteractiveDetector::NetworkActivityChecker {
 public:
  NetworkActivityCheckerForTest(Document* document)
      : InteractiveDetector::NetworkActivityChecker(document) {}

  virtual void SetActiveConnections(int active_connections) {
    active_connections_ = active_connections;
  };
  virtual int GetActiveConnections() { return active_connections_; }

 private:
  int active_connections_ = 0;
};

struct TaskTiming {
  double start;
  double end;
  TaskTiming(double start, double end) : start(start), end(end) {}
};

static const char kSupplementName[] = "InteractiveDetector";

class InteractiveDetectorTest : public ::testing::Test {
 public:
  InteractiveDetectorTest() : document_(Document::CreateForTest()) {
    InteractiveDetector* detector = new InteractiveDetector(
        *document_, new NetworkActivityCheckerForTest(document_));
    Supplement<Document>::ProvideTo(*document_, kSupplementName, detector);
    detector_ = detector;
  }

 protected:
  void SetUp() override { platform_->AdvanceClockSeconds(1); }

  Document* GetDocument() { return document_; }

  InteractiveDetector* GetDetector() { return detector_; }

  NetworkActivityCheckerForTest* GetNetworkActivityChecker() {
    // We know in this test context that network_activity_checker_ is an
    // instance of NetworkActivityCheckerForTest, so this static_cast is safe.
    return static_cast<NetworkActivityCheckerForTest*>(
        detector_->network_activity_checker_.get());
  }

  void SimulateNavigationStart(double nav_start_time) {
    RunTillTimestamp(nav_start_time);
    detector_->SetNavigationStartTime(nav_start_time);
  }

  void SimulateLongTask(TaskTiming task_timing) {
    CHECK(task_timing.end - task_timing.start >= 0.05);
    RunTillTimestamp(task_timing.end);
    detector_->OnLongTaskDetected(task_timing.start, task_timing.end);
  }

  void SimulateDOMContentLoadedEnd(double dcl_time) {
    RunTillTimestamp(dcl_time);
    detector_->OnDomContentLoadedEnd(dcl_time);
  }

  void SimulateFMPDetected(double fmp_time) {
    detector_->OnFirstMeaningfulPaintDetected(fmp_time);
  }

  void SimulateFMPDetected(double fmp_time, double detection_time) {
    RunTillTimestamp(detection_time);
    detector_->OnFirstMeaningfulPaintDetected(fmp_time);
  }

  void RunTillTimestamp(double target_time) {
    double current_time = MonotonicallyIncreasingTime();
    platform_->RunForPeriodSeconds(std::max(0.0, target_time - current_time));
  }

  int GetActiveConnections() {
    return GetNetworkActivityChecker()->GetActiveConnections();
  }

  void SetActiveConnections(int active_connections) {
    GetNetworkActivityChecker()->SetActiveConnections(active_connections);
  }

  void SimulateResourceLoadBegin(double load_begin_time) {
    RunTillTimestamp(load_begin_time);
    detector_->OnResourceLoadBegin();
    // ActiveConnections is incremented after detector runs OnResourceLoadBegin;
    SetActiveConnections(GetActiveConnections() + 1);
  }

  void SimulateResourceLoadEnd(double load_finish_time) {
    RunTillTimestamp(load_finish_time);
    int active_connections = GetActiveConnections();
    SetActiveConnections(active_connections - 1);
    detector_->OnResourceLoadEnd(load_finish_time);
  }

  double GetConsistentlyInteractiveTime() {
    return detector_->GetConsistentlyInteractiveTime();
  }

 protected:
  ScopedTestingPlatformSupport<TestingPlatformSupportWithMockScheduler>
      platform_;

  Persistent<Document> document_;
  Persistent<InteractiveDetector> detector_;
};

// Note: The tests currently assume kConsistentlyInteractiveWindowSeconds is 5
// seconds. The window size is unlikely to change, and this makes the test
// scenarios significantly easier to write.

// Note: Some of the tests are named W_X_Y_Z, where W, X, Y, Z can any of the
// following events:
// FMP: First Meaningful Paint
// DCL: DomContentLoadedEnd
// FmpDetect: Detection of FMP. FMP is not detected in realtime.
// LT: Long Task
// The name shows the ordering of these events in the test.

TEST_F(InteractiveDetectorTest, FMP_DCL_FmpDetect) {
  double initial_time = MonotonicallyIncreasingTime();
  double nav_start_time = initial_time;
  double dcl_time = initial_time + 3.0;
  double fmp_time = initial_time + 5.0;
  double fmp_detection_time = initial_time + 7.0;

  SimulateNavigationStart(nav_start_time);
  // Network is forever quiet for this test.
  SetActiveConnections(1);
  RunTillTimestamp(dcl_time);
  SimulateDOMContentLoadedEnd(dcl_time);
  RunTillTimestamp(fmp_detection_time);
  SimulateFMPDetected(fmp_time);
  RunTillTimestamp(fmp_time + 5.0 + 0.1);
  EXPECT_EQ(GetConsistentlyInteractiveTime(), fmp_time);
}

TEST_F(InteractiveDetectorTest, DCL_FMP_FmpDetect) {
  double initial_time = MonotonicallyIncreasingTime();
  double nav_start_time = initial_time;
  double fmp_time = initial_time + 3.0;
  double dcl_time = initial_time + 5.0;
  double fmp_detection_time = initial_time + 7.0;

  SimulateNavigationStart(nav_start_time);
  // Network is forever quiet for this test.
  SetActiveConnections(1);
  RunTillTimestamp(dcl_time);
  SimulateDOMContentLoadedEnd(dcl_time);
  RunTillTimestamp(fmp_detection_time);
  SimulateFMPDetected(fmp_time);
  RunTillTimestamp(fmp_time + 5.0 + 0.1);
  // At this point it has been five seconds since fmp.
  EXPECT_EQ(GetConsistentlyInteractiveTime(), dcl_time);
}

TEST_F(InteractiveDetectorTest, InstantDetectionAtFmpDetectIfPossible) {
  double initial_time = MonotonicallyIncreasingTime();
  double nav_start_time = initial_time;
  double fmp_time = initial_time + 3.0;
  double dcl_time = initial_time + 5.0;
  double fmp_detection_time = initial_time + 10.0;

  SimulateNavigationStart(nav_start_time);
  // Network is forever quiet for this test.
  SetActiveConnections(1);
  SimulateDOMContentLoadedEnd(dcl_time);
  SimulateFMPDetected(fmp_time, fmp_detection_time);
  // Although we just detected FMP, the FMP timestamp is more than
  // kConsistentlyInteractiveWindowSeconds earlier. We should instantaneously
  // have Consistently Interactive.
  EXPECT_EQ(GetConsistentlyInteractiveTime(), dcl_time);
}

TEST_F(InteractiveDetectorTest, FmpDetectFiresAfterLateLongTask) {
  double initial_time = MonotonicallyIncreasingTime();
  double nav_start_time = initial_time;
  double fmp_time = initial_time + 3.0;
  double dcl_time = initial_time + 5.0;
  TaskTiming long_task(initial_time + 9.0, initial_time + 9.1);
  double fmp_detection_time = initial_time + 10.0;

  SimulateNavigationStart(nav_start_time);
  // Network is forever quiet for this test.
  SetActiveConnections(1);
  SimulateDOMContentLoadedEnd(dcl_time);
  SimulateLongTask(long_task);
  SimulateFMPDetected(fmp_time, fmp_detection_time);
  // There is a 5 second quiet window after fmp_time - the long task is 6s
  // seconds after fmp_time. We should instantly have Consistently Interactive.
  EXPECT_EQ(GetConsistentlyInteractiveTime(), dcl_time);
}

TEST_F(InteractiveDetectorTest, FMP_FmpDetect_DCL) {
  double initial_time = MonotonicallyIncreasingTime();
  double nav_start_time = initial_time;
  double fmp_time = initial_time + 3.0;
  double fmp_detection_time = initial_time + 5.0;
  double dcl_time = initial_time + 9.0;

  SimulateNavigationStart(nav_start_time);
  // Network is forever quiet for this test.
  SetActiveConnections(1);
  RunTillTimestamp(fmp_detection_time);
  SimulateFMPDetected(fmp_time);
  RunTillTimestamp(dcl_time);
  SimulateDOMContentLoadedEnd(dcl_time);
  // It has been more than five seconds since FMP by the time DCL fired, so we
  // should instantaneously have Consistently Interactive.
  EXPECT_EQ(GetConsistentlyInteractiveTime(), dcl_time);
}

TEST_F(InteractiveDetectorTest, LongTaskBeforeFMPDoesNotAffectTTCI) {
  double initial_time = MonotonicallyIncreasingTime();
  double nav_start_time = initial_time;
  double dcl_time = initial_time + 3.0;
  TaskTiming long_task(initial_time + 5.100, initial_time + 5.250);
  double fmp_time = initial_time + 8.0;
  double fmp_detection_time = initial_time + 9.0;

  SimulateNavigationStart(nav_start_time);
  // Network is forever quiet for this test.
  SetActiveConnections(1);
  RunTillTimestamp(dcl_time);
  SimulateDOMContentLoadedEnd(dcl_time);
  RunTillTimestamp(long_task.end);
  SimulateLongTask(long_task);
  RunTillTimestamp(fmp_detection_time);
  SimulateFMPDetected(fmp_time);
  RunTillTimestamp(fmp_time + 5.0 + 0.1);
  // It has been 5s since FMP at this point.
  EXPECT_EQ(GetConsistentlyInteractiveTime(), fmp_time);
}

TEST_F(InteractiveDetectorTest, DCLDoesNotResetTimer) {
  double t0 = MonotonicallyIncreasingTime();
  double nav_start_time = t0;
  double fmp_time = t0 + 3.0;
  double fmp_detection_time = t0 + 4.0;
  TaskTiming long_task(t0 + 5.0, t0 + 5.1);
  double dcl_time = t0 + 8.0;

  SimulateNavigationStart(nav_start_time);
  // Network is forever quiet for this test.
  SetActiveConnections(1);
  // RunTillTimestamp(fmp_detection_time);
  SimulateFMPDetected(fmp_time, fmp_detection_time);
  // RunTillTimestamp(long_task.end);
  SimulateLongTask(long_task);
  // RunTillTimestamp(dcl_time);
  SimulateDOMContentLoadedEnd(dcl_time);
  RunTillTimestamp(long_task.end + 5.0 + 0.1);
  EXPECT_EQ(GetConsistentlyInteractiveTime(), dcl_time);
}

TEST_F(InteractiveDetectorTest, DCL_FMP_FmpDetect_LT) {
  double t0 = MonotonicallyIncreasingTime();
  double nav_start_time = t0;
  double dcl_time = t0 + 3.0;
  double fmp_time = t0 + 4.0;
  double fmp_detection_time = t0 + 5.0;
  TaskTiming long_task(t0 + 7.0, t0 + 7.1);

  SimulateNavigationStart(nav_start_time);
  // Network is forever quiet for this test.
  SetActiveConnections(1);
  SimulateDOMContentLoadedEnd(dcl_time);
  SimulateFMPDetected(fmp_time, fmp_detection_time);
  SimulateLongTask(long_task);
  double end_timestamp = long_task.end + 5.0 + 0.1;
  RunTillTimestamp(end_timestamp);
  EXPECT_EQ(GetConsistentlyInteractiveTime(), long_task.end);
}

TEST_F(InteractiveDetectorTest, DCL_FMP_LT_FmpDetect) {
  double t0 = MonotonicallyIncreasingTime();
  double nav_start_time = t0;
  double dcl_time = t0 + 3.0;
  double fmp_time = t0 + 4.0;
  TaskTiming long_task(t0 + 7.0, t0 + 7.1);
  double fmp_detection_time = t0 + 8.0;

  SimulateNavigationStart(nav_start_time);
  // Network is forever quiet for this test.
  SetActiveConnections(1);
  SimulateDOMContentLoadedEnd(dcl_time);
  SimulateLongTask(long_task);
  SimulateFMPDetected(fmp_time, fmp_detection_time);
  RunTillTimestamp(long_task.end + 5.0 + 0.1);
  SimulateLongTask(long_task);
  EXPECT_EQ(GetConsistentlyInteractiveTime(), long_task.end);
}

TEST_F(InteractiveDetectorTest, FMP_FmpDetect_LT_DCL) {
  double t0 = MonotonicallyIncreasingTime();
  double nav_start_time = t0;
  double fmp_time = t0 + 3.0;
  double fmp_detection_time = t0 + 4.0;
  TaskTiming long_task(t0 + 7.0, t0 + 7.1);
  double dcl_time = t0 + 8.0;

  SimulateNavigationStart(nav_start_time);
  // Network is forever quiet for this test.
  SetActiveConnections(1);
  SimulateFMPDetected(fmp_time, fmp_detection_time);
  SimulateLongTask(long_task);
  SimulateDOMContentLoadedEnd(dcl_time);
  RunTillTimestamp(long_task.end + 5.0 + 0.1);
  // Note we do not need to wait for DCL +
  // kConsistentlyInteractiveWindowSeconds.
  EXPECT_EQ(GetConsistentlyInteractiveTime(), dcl_time);
}

TEST_F(InteractiveDetectorTest, DclIsMoreThan5sAfterFMP) {
  double t0 = MonotonicallyIncreasingTime();
  double nav_start_time = t0;
  double fmp_time = t0 + 3.0;
  double fmp_detection_time = t0 + 4.0;
  TaskTiming long_task1(t0 + 7.0, t0 + 7.1);
  double dcl_time = t0 + 10.0;
  TaskTiming long_task2(t0 + 11.0, t0 + 11.1);

  SimulateNavigationStart(nav_start_time);
  // Network is forever quiet for this test.
  SetActiveConnections(1);
  SimulateFMPDetected(fmp_time, fmp_detection_time);
  SimulateLongTask(long_task1);
  SimulateDOMContentLoadedEnd(dcl_time);
  // Have not reached TTCI yet.
  EXPECT_EQ(GetConsistentlyInteractiveTime(), 0.0);
  SimulateLongTask(long_task2);
  RunTillTimestamp(long_task2.end + 5.0 + 0.1);
  EXPECT_EQ(GetConsistentlyInteractiveTime(), long_task2.end);
}

TEST_F(InteractiveDetectorTest, NetworkBusyBlocksTTCIEvenWhenMainThreadQuiet) {
  double t0 = MonotonicallyIncreasingTime();
  double nav_start_time = t0;
  double dcl_time = t0 + 2.0;
  double fmp_time = t0 + 3.0;
  double request2_start = t0 + 3.4;
  double request3_start = t0 + 3.5;  // Network busy start.
  double fmp_detection_time = t0 + 4.0;
  TaskTiming long_task_1(t0 + 7.0, t0 + 7.1);
  TaskTiming long_task_2(t0 + 13.0, t0 + 13.1);
  double network_busy_end = t0 + 12.0;

  SimulateNavigationStart(nav_start_time);
  SetActiveConnections(1);
  SimulateDOMContentLoadedEnd(dcl_time);
  SimulateResourceLoadBegin(request2_start);
  SimulateResourceLoadBegin(request3_start);
  SimulateFMPDetected(fmp_time, fmp_detection_time);
  SimulateLongTask(long_task_1);
  SimulateResourceLoadEnd(network_busy_end);
  // Network busy keeps page from reaching Consistently Interactive.
  SimulateLongTask(long_task_2);
  RunTillTimestamp(long_task_2.end + 5.0 + 0.1);
  EXPECT_EQ(GetConsistentlyInteractiveTime(), long_task_2.end);
}

TEST_F(InteractiveDetectorTest, LongEnoughQuietWindowBetweenFMPAndFmpDetect) {
  double t0 = MonotonicallyIncreasingTime();
  double nav_start_time = t0;
  double dcl_time = t0 + 2.0;
  TaskTiming long_task_1(t0 + 2.1, t0 + 2.2);
  double fmp_time = t0 + 3.0;
  TaskTiming long_task_2(t0 + 8.2, t0 + 8.3);
  double request2_start = t0 + 8.4;
  double request3_start = t0 + 8.5;  // Network busy start.
  double fmp_detection_time = t0 + 10.0;

  SimulateNavigationStart(nav_start_time);
  SetActiveConnections(1);
  SimulateDOMContentLoadedEnd(dcl_time);
  SimulateLongTask(long_task_1);
  SimulateLongTask(long_task_2);
  SimulateResourceLoadBegin(request2_start);
  SimulateResourceLoadBegin(request3_start);
  SimulateFMPDetected(fmp_time, fmp_detection_time);
  // Even though network is currently busy and we have long task finishing
  // recently, we should be able to detect that the page already achieved
  // Consistently Interactive.
  EXPECT_EQ(GetConsistentlyInteractiveTime(), fmp_time);
}

TEST_F(InteractiveDetectorTest, NetworkBusyEndIsNotConsistentlyInteractive) {
  double t0 = MonotonicallyIncreasingTime();
  double nav_start_time = t0;
  double dcl_time = t0 + 2.0;
  double fmp_time = t0 + 3.0;
  double request2_start = t0 + 3.4;
  double request3_start = t0 + 3.5;  // Network busy start.
  double fmp_detection_time = t0 + 4.0;
  TaskTiming long_task_1(t0 + 7.0, t0 + 7.1);
  TaskTiming long_task_2(t0 + 13.0, t0 + 13.1);
  double network_busy_end = t0 + 14.0;

  SimulateNavigationStart(nav_start_time);
  SetActiveConnections(1);
  SimulateDOMContentLoadedEnd(dcl_time);
  SimulateResourceLoadBegin(request2_start);
  SimulateResourceLoadBegin(request3_start);
  SimulateFMPDetected(fmp_time, fmp_detection_time);
  SimulateLongTask(long_task_1);
  SimulateLongTask(long_task_2);
  SimulateResourceLoadEnd(network_busy_end);
  RunTillTimestamp(network_busy_end + 5.0 + 0.1);
  EXPECT_NE(GetConsistentlyInteractiveTime(), network_busy_end);
  EXPECT_EQ(GetConsistentlyInteractiveTime(), long_task_2.end);
}

TEST_F(InteractiveDetectorTest, LongEnoughNetworkQuietBeforeLateLongTask) {
  double t0 = MonotonicallyIncreasingTime();
  double nav_start_time = t0;
  double dcl_time = t0 + 2.0;
  double fmp_time = t0 + 3.0;
  double request2_start = t0 + 3.4;
  double request3_start = t0 + 3.5;  // Network busy start.
  double fmp_detection_time = t0 + 4.0;
  TaskTiming long_task_1(t0 + 7.0, t0 + 7.1);
  double network_busy_end = t0 + 8.0;
  // Long gap before next long task.
  TaskTiming long_task_2(t0 + 14.0, t0 + 14.1);

  SimulateNavigationStart(nav_start_time);
  SetActiveConnections(1);
  SimulateDOMContentLoadedEnd(dcl_time);
  SimulateResourceLoadBegin(request2_start);
  SimulateResourceLoadBegin(request3_start);
  SimulateFMPDetected(fmp_time, fmp_detection_time);
  SimulateLongTask(long_task_1);
  SimulateResourceLoadEnd(network_busy_end);
  SimulateLongTask(long_task_2);
  // Running till very late to make sure Consistently Interactive value doesn't
  // change even if it is already detected.
  RunTillTimestamp(long_task_2.end + 5.0 + 0.1);
  EXPECT_NE(GetConsistentlyInteractiveTime(), long_task_2.end);
  EXPECT_EQ(GetConsistentlyInteractiveTime(), long_task_1.end);
}

TEST_F(InteractiveDetectorTest, LateLongTaskWithLateFMPDetection) {
  double t0 = MonotonicallyIncreasingTime();
  double nav_start_time = t0;
  double dcl_time = t0 + 2.0;
  double fmp_time = t0 + 3.0;
  double request2_start = t0 + 3.4;
  double request3_start = t0 + 3.5;  // Network busy start.
  TaskTiming long_task_1(t0 + 7.0, t0 + 7.1);
  double network_busy_end = t0 + 8.0;
  // Long gap before next long task.
  TaskTiming long_task_2(t0 + 14.0, t0 + 14.1);
  double fmp_detection_time = t0 + 20.0;

  SimulateNavigationStart(nav_start_time);
  SetActiveConnections(1);
  SimulateDOMContentLoadedEnd(dcl_time);
  SimulateResourceLoadBegin(request2_start);
  SimulateResourceLoadBegin(request3_start);
  SimulateLongTask(long_task_1);
  SimulateResourceLoadEnd(network_busy_end);
  SimulateLongTask(long_task_2);
  SimulateFMPDetected(fmp_time, fmp_detection_time);
  EXPECT_NE(GetConsistentlyInteractiveTime(), long_task_2.end);
  EXPECT_EQ(GetConsistentlyInteractiveTime(), long_task_1.end);
}

TEST_F(InteractiveDetectorTest, IntermittentNetworkBusyBlocksTTCI) {
  double t0 = MonotonicallyIncreasingTime();
  double nav_start_time = t0;
  double dcl_time = t0 + 2.0;
  double fmp_time = t0 + 3.0;
  double fmp_detection_time = t0 + 4.0;
  TaskTiming long_task_1(t0 + 7.0, t0 + 7.1);
  double request2_start = t0 + 7.9;
  // Network goes busy and quiet twice.
  double network_busy_start_1 = t0 + 8.0;
  double network_busy_end_1 = t0 + 8.5;
  double network_busy_start_2 = t0 + 11.0;
  double network_busy_end_2 = t0 + 12.0;
  // Late long task.
  TaskTiming long_task_2(t0 + 14.0, t0 + 14.1);

  SimulateNavigationStart(nav_start_time);
  SetActiveConnections(1);
  SimulateDOMContentLoadedEnd(dcl_time);
  SimulateFMPDetected(fmp_time, fmp_detection_time);
  SimulateLongTask(long_task_1);
  SimulateResourceLoadBegin(request2_start);
  SimulateResourceLoadBegin(network_busy_start_1);
  SimulateResourceLoadEnd(network_busy_end_1);
  SimulateResourceLoadBegin(network_busy_start_2);
  SimulateResourceLoadEnd(network_busy_end_2);
  SimulateLongTask(long_task_2);
  RunTillTimestamp(long_task_2.end + 5.0 + 0.1);
  EXPECT_EQ(GetConsistentlyInteractiveTime(), long_task_2.end);
}

class InteractiveDetectorTestWithDummyPage : public ::testing::Test {
 public:
  // Public because it's executed on a task queue.
  void DummyTaskWithDuration(double duration_seconds) {
    platform_->AdvanceClockSeconds(duration_seconds);
    dummy_task_end_time_ = MonotonicallyIncreasingTime();
  }

 protected:
  void SetUp() override {
    platform_->AdvanceClockSeconds(1);
    dummy_page_holder_ = DummyPageHolder::Create(IntSize(800, 600));
  }

  double GetDummyTaskEndTime() { return dummy_task_end_time_; }

  Document& GetDocument() { return dummy_page_holder_->GetDocument(); }
  ScopedTestingPlatformSupport<TestingPlatformSupportWithMockScheduler>
      platform_;

 private:
  std::unique_ptr<DummyPageHolder> dummy_page_holder_;
  double dummy_task_end_time_ = 0.0;
};

TEST_F(InteractiveDetectorTestWithDummyPage, TaskLongerThan5sBlocksTTCI) {
  double initial_time = MonotonicallyIncreasingTime();
  InteractiveDetector* detector = InteractiveDetector::From(GetDocument());
  detector->SetNavigationStartTime(initial_time);
  platform_->RunForPeriodSeconds(4.0);

  // DummyPageHolder automatically fires DomContentLoadedEnd, but not First
  // Meaningful Paint. We therefore manually Invoking the listener on
  // InteractiveDetector.
  detector->OnFirstMeaningfulPaintDetected(initial_time + 3.0);

  // Post a task with 6 seconds duration.
  platform_->CurrentThread()->GetWebTaskRunner()->PostTask(
      BLINK_FROM_HERE,
      CrossThreadBind(
          &InteractiveDetectorTestWithDummyPage::DummyTaskWithDuration,
          CrossThreadUnretained(this), 6.0));
  platform_->RunUntilIdle();

  // We should be able to detect TTCI 5s after the end of long task.
  platform_->RunForPeriodSeconds(5.1);
  EXPECT_EQ(detector->GetConsistentlyInteractiveTime(), GetDummyTaskEndTime());
}

}  // namespace blink
