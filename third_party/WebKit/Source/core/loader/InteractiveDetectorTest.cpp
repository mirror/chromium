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

  void SimulateTaskEnd(TaskTiming task_timing) {
    RunTillTimestamp(task_timing.end);
    detector_->DidProcessTask(task_timing.start, task_timing.end);
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

  void SetActiveConnections(int active_connections, double timestamp) {
    RunTillTimestamp(timestamp);
    GetNetworkActivityChecker()->SetActiveConnections(active_connections);
  }

  void SimulateResourceLoadEnd(double load_finish_time) {
    RunTillTimestamp(load_finish_time);
    int active_connections = GetActiveConnections();
    SetActiveConnections(active_connections - 1);
    detector_->OnResourceLoadEnd(load_finish_time);
  }

  void SimulateResourceLoadStart(double load_start_time) {
    RunTillTimestamp(load_start_time);
    int active_connections = GetActiveConnections();
    SetActiveConnections(active_connections + 1);
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

// TODO(dproy): The tests currently assume kConsistentlyInteractiveWindowSeconds
// is 5 seconds. It may be worth it to make these tests work for any
// kConsistentlyInteractiveWindowSeconds.

// Note: Some of the tests are named W_X_Y_Z, where W, X, Y, Z can any of the
// following events:
// FMP: First Meaningful Paint
// DCL: DomContentLoadedEnd
// FmpDetect: Detection of FMP. FMP is not detected in realtime.
// LT: Long Task
// And sometimes, ST: Short Task
//
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

TEST_F(InteractiveDetectorTest, FmpDetectFiresLongAfterFmp) {
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
  SimulateTaskEnd(long_task);
  RunTillTimestamp(fmp_detection_time);
  SimulateFMPDetected(fmp_time);
  RunTillTimestamp(fmp_time + 5.0 + 0.1);
  // It has been 5s since FMP at this point.
  EXPECT_EQ(GetConsistentlyInteractiveTime(), fmp_time);
}

TEST_F(InteractiveDetectorTest, ShortTasksDoNotAffectTTCI) {
  double t0 = MonotonicallyIncreasingTime();
  double nav_start_time = t0;
  double dcl_time = t0 + 3.0;
  double fmp_time = t0 + 5.0;
  // 10ms task.
  TaskTiming short_task_1(t0 + 5.700, t0 + 5.710);
  // 49ms task.
  TaskTiming short_task_2(t0 + 6.500, t0 + 6.549);
  double fmp_detection_time = t0 + 7.0;

  SimulateNavigationStart(nav_start_time);
  // Network is forever quiet for this test.
  SetActiveConnections(1);
  RunTillTimestamp(dcl_time);
  SimulateDOMContentLoadedEnd(dcl_time);
  RunTillTimestamp(short_task_1.end);
  SimulateTaskEnd(short_task_1);
  RunTillTimestamp(short_task_2.end);
  SimulateTaskEnd(short_task_2);
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
  SimulateTaskEnd(long_task);
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
  SimulateTaskEnd(long_task);
  RunTillTimestamp(long_task.end + 5.0 + 0.1);
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
  SimulateTaskEnd(long_task);
  SimulateFMPDetected(fmp_time, fmp_detection_time);
  RunTillTimestamp(long_task.end + 5.0 + 0.1);
  SimulateTaskEnd(long_task);
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
  SimulateTaskEnd(long_task);
  SimulateDOMContentLoadedEnd(dcl_time);
  RunTillTimestamp(long_task.end + 5.0 + 0.1);
  // Note we do not need to wait for DCL +
  // kConsistentlyInteractiveWindowSeconds.
  EXPECT_EQ(GetConsistentlyInteractiveTime(), dcl_time);
}

TEST_F(InteractiveDetectorTest, NetworkBusyBlocksTTCIEvenWhenMainThreadQuiet) {
  double t0 = MonotonicallyIncreasingTime();
  double nav_start_time = t0;
  double dcl_time = t0 + 2.0;
  double fmp_time = t0 + 3.0;
  double network_busy_start = t0 + 3.5;
  double fmp_detection_time = t0 + 4.0;
  TaskTiming long_task_1(t0 + 7.0, t0 + 7.1);
  TaskTiming long_task_2(t0 + 13.0, t0 + 13.1);
  double network_busy_end = t0 + 12.0;

  SimulateNavigationStart(nav_start_time);
  SetActiveConnections(1);
  SimulateDOMContentLoadedEnd(dcl_time);
  SetActiveConnections(3, network_busy_start);
  SimulateFMPDetected(fmp_time, fmp_detection_time);
  SimulateTaskEnd(long_task_1);
  SimulateResourceLoadEnd(network_busy_end);
  // Network busy keeps page from reaching Consistently Interactive.
  SimulateTaskEnd(long_task_2);
  RunTillTimestamp(long_task_2.end + 5.0 + 0.1);
  EXPECT_EQ(GetConsistentlyInteractiveTime(), long_task_2.end);
}

TEST_F(InteractiveDetectorTest, NetworkBusyEndIsNotConsistentlyInteractive) {
  double t0 = MonotonicallyIncreasingTime();
  double nav_start_time = t0;
  double dcl_time = t0 + 2.0;
  double fmp_time = t0 + 3.0;
  double network_busy_start = t0 + 3.5;
  double fmp_detection_time = t0 + 4.0;
  TaskTiming long_task_1(t0 + 7.0, t0 + 7.1);
  TaskTiming long_task_2(t0 + 13.0, t0 + 13.1);
  double network_busy_end = t0 + 14.0;

  SimulateNavigationStart(nav_start_time);
  SetActiveConnections(1);
  SimulateDOMContentLoadedEnd(dcl_time);
  SetActiveConnections(3, network_busy_start);
  SimulateFMPDetected(fmp_time, fmp_detection_time);
  SimulateTaskEnd(long_task_1);
  SimulateTaskEnd(long_task_2);
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
  double network_busy_start = t0 + 3.5;
  double fmp_detection_time = t0 + 4.0;
  TaskTiming long_task_1(t0 + 7.0, t0 + 7.1);
  double network_busy_end = t0 + 8.0;
  // Long gap before next long task.
  TaskTiming long_task_2(t0 + 14.0, t0 + 14.1);

  SimulateNavigationStart(nav_start_time);
  SetActiveConnections(1);
  SimulateDOMContentLoadedEnd(dcl_time);
  SetActiveConnections(3, network_busy_start);
  SimulateFMPDetected(fmp_time, fmp_detection_time);
  SimulateTaskEnd(long_task_1);
  SimulateResourceLoadEnd(network_busy_end);
  SimulateTaskEnd(long_task_2);
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
  double network_busy_start = t0 + 3.5;
  TaskTiming long_task_1(t0 + 7.0, t0 + 7.1);
  double network_busy_end = t0 + 8.0;
  // Long gap before next long task.
  TaskTiming long_task_2(t0 + 14.0, t0 + 14.1);
  double fmp_detection_time = t0 + 20.0;

  SimulateNavigationStart(nav_start_time);
  SetActiveConnections(1);
  SimulateDOMContentLoadedEnd(dcl_time);
  SetActiveConnections(3, network_busy_start);
  SimulateTaskEnd(long_task_1);
  SimulateResourceLoadEnd(network_busy_end);
  SimulateTaskEnd(long_task_2);
  SimulateFMPDetected(fmp_time, fmp_detection_time);
  EXPECT_NE(GetConsistentlyInteractiveTime(), long_task_2.end);
  EXPECT_EQ(GetConsistentlyInteractiveTime(), long_task_1.end);
}

TEST_F(InteractiveDetectorTest, IntermittentNetworkBusy) {
  double t0 = MonotonicallyIncreasingTime();
  double nav_start_time = t0;
  double dcl_time = t0 + 2.0;
  double fmp_time = t0 + 3.0;
  double fmp_detection_time = t0 + 4.0;
  TaskTiming long_task_1(t0 + 7.0, t0 + 7.1);
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
  SimulateTaskEnd(long_task_1);
  SimulateResourceLoadStart(network_busy_start_1);
  SimulateResourceLoadEnd(network_busy_end_1);
  SimulateResourceLoadStart(network_busy_start_2);
  SimulateResourceLoadEnd(network_busy_end_2);
  SimulateTaskEnd(long_task_2);
  RunTillTimestamp(long_task_2.end + 5.0 + 0.1);
  EXPECT_NE(GetConsistentlyInteractiveTime(), long_task_2.end);
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
