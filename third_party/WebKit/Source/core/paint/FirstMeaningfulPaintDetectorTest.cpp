// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/paint/FirstMeaningfulPaintDetector.h"

#include "core/paint/PaintEvent.h"
#include "core/paint/PaintTiming.h"
#include "core/testing/PageTestBase.h"
#include "platform/testing/TestingPlatformSupportWithMockScheduler.h"
#include "platform/wtf/Time.h"
#include "platform/wtf/text/StringBuilder.h"
#include "public/platform/WebLayerTreeView.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace blink {

class FirstMeaningfulPaintDetectorTest : public PageTestBase {
 protected:
  void SetUp() override {
    platform_->AdvanceClockSeconds(1);
    PageTestBase::SetUp();
  }

  double AdvanceClockAndGetTime() {
    platform_->AdvanceClockSeconds(1);
    return CurrentTimeTicksInSeconds();
  }

  PaintTiming& GetPaintTiming() { return PaintTiming::From(GetDocument()); }
  FirstMeaningfulPaintDetector& Detector() {
    return GetPaintTiming().GetFirstMeaningfulPaintDetector();
  }

  void SimulateLayoutAndPaint(int new_elements) {
    platform_->AdvanceClockSeconds(0.001);
    StringBuilder builder;
    for (int i = 0; i < new_elements; i++)
      builder.Append("<span>a</span>");
    GetDocument().write(builder.ToString());
    GetDocument().UpdateStyleAndLayout();
    Detector().NotifyPaint();
  }

  void SimulateNetworkStable() {
    GetDocument().SetParsingState(Document::kFinishedParsing);
    Detector().Network0QuietTimerFired(nullptr);
    Detector().Network2QuietTimerFired(nullptr);
  }

  void SimulateNetwork0Quiet() {
    GetDocument().SetParsingState(Document::kFinishedParsing);
    Detector().Network0QuietTimerFired(nullptr);
  }

  void SimulateNetwork2Quiet() {
    GetDocument().SetParsingState(Document::kFinishedParsing);
    Detector().Network2QuietTimerFired(nullptr);
  }

  void SimulateUserInput() { Detector().NotifyInputEvent(); }

  void SetActiveConnections(int connections) {
    Detector().SetNetworkQuietTimers(connections);
  }

  bool IsNetwork0QuietTimerActive() {
    return Detector().network0_quiet_timer_.IsActive();
  }

  bool IsNetwork2QuietTimerActive() {
    return Detector().network2_quiet_timer_.IsActive();
  }

  bool HadNetwork0Quiet() { return Detector().network0_quiet_reached_; }
  bool HadNetwork2Quiet() { return Detector().network2_quiet_reached_; }

  void ClearFirstPaintSwapPromise() {
    platform_->AdvanceClockSeconds(0.001);
    GetPaintTiming().ReportSwapTime(PaintEvent::kFirstPaint,
                                    WebLayerTreeView::SwapResult::kDidSwap,
                                    CurrentTimeTicksInSeconds());
  }

  void ClearFirstContentfulPaintSwapPromise() {
    platform_->AdvanceClockSeconds(0.001);
    GetPaintTiming().ReportSwapTime(PaintEvent::kFirstContentfulPaint,
                                    WebLayerTreeView::SwapResult::kDidSwap,
                                    CurrentTimeTicksInSeconds());
  }

  void ClearProvisionalFirstMeaningfulPaintSwapPromise() {
    platform_->AdvanceClockSeconds(0.001);
    ClearProvisionalFirstMeaningfulPaintSwapPromise(
        CurrentTimeTicksInSeconds());
  }

  void ClearProvisionalFirstMeaningfulPaintSwapPromise(double timestamp) {
    Detector().ReportSwapTime(PaintEvent::kProvisionalFirstMeaningfulPaint,
                              WebLayerTreeView::SwapResult::kDidSwap,
                              timestamp);
  }

  unsigned OutstandingDetectorSwapPromiseCount() {
    return Detector().outstanding_swap_promise_count_;
  }

  void MarkFirstContentfulPaintAndClearSwapPromise() {
    GetPaintTiming().MarkFirstContentfulPaint();
    ClearFirstContentfulPaintSwapPromise();
  }

  void MarkFirstPaintAndClearSwapPromise() {
    GetPaintTiming().MarkFirstPaint();
    ClearFirstPaintSwapPromise();
  }

 protected:
  ScopedTestingPlatformSupport<TestingPlatformSupportWithMockScheduler>
      platform_;

  static constexpr double kNetwork0QuietWindowSeconds =
      FirstMeaningfulPaintDetector::kNetwork0QuietWindowSeconds;
  static constexpr double kNetwork2QuietWindowSeconds =
      FirstMeaningfulPaintDetector::kNetwork2QuietWindowSeconds;
};

TEST_F(FirstMeaningfulPaintDetectorTest, NoFirstPaint) {
  SimulateLayoutAndPaint(1);
  EXPECT_EQ(OutstandingDetectorSwapPromiseCount(), 0U);
  SimulateNetworkStable();
  EXPECT_EQ(TimeTicksInSeconds(GetPaintTiming().FirstMeaningfulPaintRendered()),
            0.0);
  EXPECT_EQ(TimeTicksInSeconds(GetPaintTiming().FirstMeaningfulPaint()), 0.0);
}

TEST_F(FirstMeaningfulPaintDetectorTest, OneLayout) {
  MarkFirstContentfulPaintAndClearSwapPromise();
  SimulateLayoutAndPaint(1);
  EXPECT_EQ(OutstandingDetectorSwapPromiseCount(), 1U);
  ClearProvisionalFirstMeaningfulPaintSwapPromise();
  double after_paint = AdvanceClockAndGetTime();
  EXPECT_EQ(TimeTicksInSeconds(GetPaintTiming().FirstMeaningfulPaintRendered()),
            0.0);
  EXPECT_EQ(TimeTicksInSeconds(GetPaintTiming().FirstMeaningfulPaint()), 0.0);
  SimulateNetworkStable();
  EXPECT_GT(TimeTicksInSeconds(GetPaintTiming().FirstMeaningfulPaintRendered()),
            TimeTicksInSeconds(GetPaintTiming().FirstPaintRendered()));
  EXPECT_GT(
      TimeTicksInSeconds(GetPaintTiming().FirstMeaningfulPaint()),
      TimeTicksInSeconds(GetPaintTiming().FirstMeaningfulPaintRendered()));
  EXPECT_LT(TimeTicksInSeconds(GetPaintTiming().FirstMeaningfulPaintRendered()),
            after_paint);
  EXPECT_LT(TimeTicksInSeconds(GetPaintTiming().FirstMeaningfulPaint()),
            after_paint);
}

TEST_F(FirstMeaningfulPaintDetectorTest, TwoLayoutsSignificantSecond) {
  MarkFirstContentfulPaintAndClearSwapPromise();
  SimulateLayoutAndPaint(1);
  EXPECT_EQ(OutstandingDetectorSwapPromiseCount(), 1U);
  ClearProvisionalFirstMeaningfulPaintSwapPromise();
  double after_layout1 = AdvanceClockAndGetTime();
  SimulateLayoutAndPaint(10);
  EXPECT_EQ(OutstandingDetectorSwapPromiseCount(), 1U);
  ClearProvisionalFirstMeaningfulPaintSwapPromise();
  double after_layout2 = AdvanceClockAndGetTime();
  SimulateNetworkStable();
  EXPECT_GT(TimeTicksInSeconds(GetPaintTiming().FirstMeaningfulPaintRendered()),
            after_layout1);
  EXPECT_GT(TimeTicksInSeconds(GetPaintTiming().FirstMeaningfulPaint()),
            after_layout1);
  EXPECT_LT(TimeTicksInSeconds(GetPaintTiming().FirstMeaningfulPaintRendered()),
            after_layout2);
  EXPECT_LT(TimeTicksInSeconds(GetPaintTiming().FirstMeaningfulPaint()),
            after_layout2);
  EXPECT_GT(
      TimeTicksInSeconds(GetPaintTiming().FirstMeaningfulPaint()),
      TimeTicksInSeconds(GetPaintTiming().FirstMeaningfulPaintRendered()));
}

TEST_F(FirstMeaningfulPaintDetectorTest, TwoLayoutsSignificantFirst) {
  MarkFirstContentfulPaintAndClearSwapPromise();
  SimulateLayoutAndPaint(10);
  EXPECT_EQ(OutstandingDetectorSwapPromiseCount(), 1U);
  ClearProvisionalFirstMeaningfulPaintSwapPromise();
  double after_layout1 = AdvanceClockAndGetTime();
  SimulateLayoutAndPaint(1);
  EXPECT_EQ(OutstandingDetectorSwapPromiseCount(), 0U);
  SimulateNetworkStable();
  EXPECT_GT(TimeTicksInSeconds(GetPaintTiming().FirstMeaningfulPaintRendered()),
            TimeTicksInSeconds(GetPaintTiming().FirstPaintRendered()));
  EXPECT_GT(TimeTicksInSeconds(GetPaintTiming().FirstMeaningfulPaint()),
            TimeTicksInSeconds(GetPaintTiming().FirstPaintRendered()));
  EXPECT_LT(TimeTicksInSeconds(GetPaintTiming().FirstMeaningfulPaintRendered()),
            after_layout1);
  EXPECT_LT(TimeTicksInSeconds(GetPaintTiming().FirstMeaningfulPaint()),
            after_layout1);
}

TEST_F(FirstMeaningfulPaintDetectorTest, FirstMeaningfulPaintCandidate) {
  MarkFirstContentfulPaintAndClearSwapPromise();
  EXPECT_EQ(
      TimeTicksInSeconds(GetPaintTiming().FirstMeaningfulPaintCandidate()),
      0.0);
  SimulateLayoutAndPaint(1);
  EXPECT_EQ(OutstandingDetectorSwapPromiseCount(), 1U);
  ClearProvisionalFirstMeaningfulPaintSwapPromise();
  double after_paint = AdvanceClockAndGetTime();
  // The first candidate gets ignored.
  EXPECT_EQ(
      TimeTicksInSeconds(GetPaintTiming().FirstMeaningfulPaintCandidate()),
      0.0);
  SimulateLayoutAndPaint(10);
  EXPECT_EQ(OutstandingDetectorSwapPromiseCount(), 1U);
  ClearProvisionalFirstMeaningfulPaintSwapPromise();
  // The second candidate gets reported.
  EXPECT_GT(
      TimeTicksInSeconds(GetPaintTiming().FirstMeaningfulPaintCandidate()),
      after_paint);
  double candidate =
      TimeTicksInSeconds(GetPaintTiming().FirstMeaningfulPaintCandidate());
  // The third candidate gets ignored since we already saw the first candidate.
  SimulateLayoutAndPaint(20);
  EXPECT_EQ(OutstandingDetectorSwapPromiseCount(), 1U);
  ClearProvisionalFirstMeaningfulPaintSwapPromise();
  EXPECT_EQ(
      TimeTicksInSeconds(GetPaintTiming().FirstMeaningfulPaintCandidate()),
      candidate);
}

TEST_F(FirstMeaningfulPaintDetectorTest,
       OnlyOneFirstMeaningfulPaintCandidateBeforeNetworkStable) {
  MarkFirstContentfulPaintAndClearSwapPromise();
  EXPECT_EQ(
      TimeTicksInSeconds(GetPaintTiming().FirstMeaningfulPaintCandidate()),
      0.0);
  double before_paint = AdvanceClockAndGetTime();
  SimulateLayoutAndPaint(1);
  EXPECT_EQ(OutstandingDetectorSwapPromiseCount(), 1U);
  ClearProvisionalFirstMeaningfulPaintSwapPromise();
  // The first candidate is initially ignored.
  EXPECT_EQ(
      TimeTicksInSeconds(GetPaintTiming().FirstMeaningfulPaintCandidate()),
      0.0);
  SimulateNetworkStable();
  // The networkStable then promotes the first candidate.
  EXPECT_GT(
      TimeTicksInSeconds(GetPaintTiming().FirstMeaningfulPaintCandidate()),
      before_paint);
  double candidate =
      TimeTicksInSeconds(GetPaintTiming().FirstMeaningfulPaintCandidate());
  // The second candidate is then ignored.
  SimulateLayoutAndPaint(10);
  EXPECT_EQ(OutstandingDetectorSwapPromiseCount(), 0U);
  EXPECT_EQ(
      TimeTicksInSeconds(GetPaintTiming().FirstMeaningfulPaintCandidate()),
      candidate);
}

TEST_F(FirstMeaningfulPaintDetectorTest,
       NetworkStableBeforeFirstContentfulPaint) {
  MarkFirstPaintAndClearSwapPromise();
  SimulateLayoutAndPaint(1);
  EXPECT_EQ(OutstandingDetectorSwapPromiseCount(), 1U);
  ClearProvisionalFirstMeaningfulPaintSwapPromise();
  SimulateNetworkStable();
  EXPECT_EQ(TimeTicksInSeconds(GetPaintTiming().FirstMeaningfulPaintRendered()),
            0.0);
  EXPECT_EQ(TimeTicksInSeconds(GetPaintTiming().FirstMeaningfulPaint()), 0.0);
  MarkFirstContentfulPaintAndClearSwapPromise();
  SimulateNetworkStable();
  EXPECT_NE(TimeTicksInSeconds(GetPaintTiming().FirstMeaningfulPaintRendered()),
            0.0);
  EXPECT_NE(TimeTicksInSeconds(GetPaintTiming().FirstMeaningfulPaint()), 0.0);
  EXPECT_GT(
      TimeTicksInSeconds(GetPaintTiming().FirstMeaningfulPaint()),
      TimeTicksInSeconds(GetPaintTiming().FirstMeaningfulPaintRendered()));
}

TEST_F(FirstMeaningfulPaintDetectorTest,
       FirstMeaningfulPaintShouldNotBeBeforeFirstContentfulPaint) {
  MarkFirstPaintAndClearSwapPromise();
  SimulateLayoutAndPaint(10);
  EXPECT_EQ(OutstandingDetectorSwapPromiseCount(), 1U);
  ClearProvisionalFirstMeaningfulPaintSwapPromise();
  platform_->AdvanceClockSeconds(0.001);
  MarkFirstContentfulPaintAndClearSwapPromise();
  SimulateNetworkStable();
  EXPECT_GE(
      TimeTicksInSeconds(GetPaintTiming().FirstMeaningfulPaintRendered()),
      TimeTicksInSeconds(GetPaintTiming().FirstContentfulPaintRendered()));
  EXPECT_GE(TimeTicksInSeconds(GetPaintTiming().FirstMeaningfulPaint()),
            TimeTicksInSeconds(GetPaintTiming().FirstContentfulPaint()));
  EXPECT_GT(
      TimeTicksInSeconds(GetPaintTiming().FirstMeaningfulPaint()),
      TimeTicksInSeconds(GetPaintTiming().FirstMeaningfulPaintRendered()));
}

TEST_F(FirstMeaningfulPaintDetectorTest, Network2QuietThen0Quiet) {
  MarkFirstContentfulPaintAndClearSwapPromise();

  SimulateLayoutAndPaint(1);
  EXPECT_EQ(OutstandingDetectorSwapPromiseCount(), 1U);
  double after_first_paint = AdvanceClockAndGetTime();
  ClearProvisionalFirstMeaningfulPaintSwapPromise();
  double after_first_paint_swap = AdvanceClockAndGetTime();
  SimulateNetwork2Quiet();

  SimulateLayoutAndPaint(10);
  EXPECT_EQ(OutstandingDetectorSwapPromiseCount(), 0U);
  SimulateNetwork0Quiet();

  // The first paint is FirstMeaningfulPaint.
  EXPECT_GT(TimeTicksInSeconds(GetPaintTiming().FirstMeaningfulPaintRendered()),
            0.0);
  EXPECT_GT(TimeTicksInSeconds(GetPaintTiming().FirstMeaningfulPaint()), 0.0);
  EXPECT_LT(TimeTicksInSeconds(GetPaintTiming().FirstMeaningfulPaintRendered()),
            after_first_paint);
  EXPECT_GT(TimeTicksInSeconds(GetPaintTiming().FirstMeaningfulPaint()),
            after_first_paint);
  EXPECT_LT(TimeTicksInSeconds(GetPaintTiming().FirstMeaningfulPaint()),
            after_first_paint_swap);
  EXPECT_GT(
      TimeTicksInSeconds(GetPaintTiming().FirstMeaningfulPaint()),
      TimeTicksInSeconds(GetPaintTiming().FirstMeaningfulPaintRendered()));
}

TEST_F(FirstMeaningfulPaintDetectorTest, Network0QuietThen2Quiet) {
  MarkFirstContentfulPaintAndClearSwapPromise();

  SimulateLayoutAndPaint(1);
  EXPECT_EQ(OutstandingDetectorSwapPromiseCount(), 1U);
  ClearProvisionalFirstMeaningfulPaintSwapPromise();
  double after_first_paint = AdvanceClockAndGetTime();
  SimulateNetwork0Quiet();

  SimulateLayoutAndPaint(10);
  EXPECT_EQ(OutstandingDetectorSwapPromiseCount(), 1U);
  ClearProvisionalFirstMeaningfulPaintSwapPromise();
  double after_second_paint = AdvanceClockAndGetTime();
  SimulateNetwork2Quiet();

  // The second paint is FirstMeaningfulPaint.
  EXPECT_GT(TimeTicksInSeconds(GetPaintTiming().FirstMeaningfulPaintRendered()),
            after_first_paint);
  EXPECT_GT(TimeTicksInSeconds(GetPaintTiming().FirstMeaningfulPaint()),
            after_first_paint);
  EXPECT_LT(TimeTicksInSeconds(GetPaintTiming().FirstMeaningfulPaintRendered()),
            after_second_paint);
  EXPECT_LT(TimeTicksInSeconds(GetPaintTiming().FirstMeaningfulPaint()),
            after_second_paint);
  EXPECT_GT(
      TimeTicksInSeconds(GetPaintTiming().FirstMeaningfulPaint()),
      TimeTicksInSeconds(GetPaintTiming().FirstMeaningfulPaintRendered()));
  EXPECT_GT(
      TimeTicksInSeconds(GetPaintTiming().FirstMeaningfulPaint()),
      TimeTicksInSeconds(GetPaintTiming().FirstMeaningfulPaintRendered()));
}

TEST_F(FirstMeaningfulPaintDetectorTest, Network0QuietTimer) {
  MarkFirstContentfulPaintAndClearSwapPromise();

  SetActiveConnections(1);
  EXPECT_FALSE(IsNetwork0QuietTimerActive());

  SetActiveConnections(0);
  platform_->RunForPeriodSeconds(kNetwork0QuietWindowSeconds - 0.1);
  EXPECT_TRUE(IsNetwork0QuietTimerActive());
  EXPECT_FALSE(HadNetwork0Quiet());

  SetActiveConnections(0);  // This should reset the 0-quiet timer.
  platform_->RunForPeriodSeconds(kNetwork0QuietWindowSeconds - 0.1);
  EXPECT_TRUE(IsNetwork0QuietTimerActive());
  EXPECT_FALSE(HadNetwork0Quiet());

  platform_->RunForPeriodSeconds(0.1001);
  EXPECT_TRUE(HadNetwork0Quiet());
}

TEST_F(FirstMeaningfulPaintDetectorTest, Network2QuietTimer) {
  MarkFirstContentfulPaintAndClearSwapPromise();

  SetActiveConnections(3);
  EXPECT_FALSE(IsNetwork2QuietTimerActive());

  SetActiveConnections(2);
  platform_->RunForPeriodSeconds(kNetwork2QuietWindowSeconds - 0.1);
  EXPECT_TRUE(IsNetwork2QuietTimerActive());
  EXPECT_FALSE(HadNetwork2Quiet());

  SetActiveConnections(2);  // This should reset the 2-quiet timer.
  platform_->RunForPeriodSeconds(kNetwork2QuietWindowSeconds - 0.1);
  EXPECT_TRUE(IsNetwork2QuietTimerActive());
  EXPECT_FALSE(HadNetwork2Quiet());

  SetActiveConnections(1);  // This should not reset the 2-quiet timer.
  platform_->RunForPeriodSeconds(0.1001);
  EXPECT_TRUE(HadNetwork2Quiet());
}

TEST_F(FirstMeaningfulPaintDetectorTest,
       FirstMeaningfulPaintAfterUserInteraction) {
  MarkFirstContentfulPaintAndClearSwapPromise();
  SimulateUserInput();
  SimulateLayoutAndPaint(10);
  EXPECT_EQ(OutstandingDetectorSwapPromiseCount(), 1U);
  ClearProvisionalFirstMeaningfulPaintSwapPromise();
  SimulateNetworkStable();
  EXPECT_EQ(TimeTicksInSeconds(GetPaintTiming().FirstMeaningfulPaintRendered()),
            0.0);
  EXPECT_EQ(TimeTicksInSeconds(GetPaintTiming().FirstMeaningfulPaint()), 0.0);
}

TEST_F(FirstMeaningfulPaintDetectorTest, UserInteractionBeforeFirstPaint) {
  SimulateUserInput();
  MarkFirstContentfulPaintAndClearSwapPromise();
  SimulateLayoutAndPaint(10);
  EXPECT_EQ(OutstandingDetectorSwapPromiseCount(), 1U);
  ClearProvisionalFirstMeaningfulPaintSwapPromise();
  SimulateNetworkStable();
  EXPECT_NE(TimeTicksInSeconds(GetPaintTiming().FirstMeaningfulPaintRendered()),
            0.0);
  EXPECT_NE(TimeTicksInSeconds(GetPaintTiming().FirstMeaningfulPaint()), 0.0);
  EXPECT_GT(
      TimeTicksInSeconds(GetPaintTiming().FirstMeaningfulPaint()),
      TimeTicksInSeconds(GetPaintTiming().FirstMeaningfulPaintRendered()));
}

TEST_F(FirstMeaningfulPaintDetectorTest,
       WaitForSingleOutstandingSwapPromiseAfterNetworkStable) {
  MarkFirstContentfulPaintAndClearSwapPromise();
  SimulateLayoutAndPaint(10);
  EXPECT_EQ(OutstandingDetectorSwapPromiseCount(), 1U);
  SimulateNetworkStable();
  EXPECT_EQ(TimeTicksInSeconds(GetPaintTiming().FirstMeaningfulPaintRendered()),
            0.0);
  EXPECT_EQ(TimeTicksInSeconds(GetPaintTiming().FirstMeaningfulPaint()), 0.0);
  ClearProvisionalFirstMeaningfulPaintSwapPromise();
  EXPECT_NE(TimeTicksInSeconds(GetPaintTiming().FirstMeaningfulPaintRendered()),
            0.0);
  EXPECT_NE(TimeTicksInSeconds(GetPaintTiming().FirstMeaningfulPaint()), 0.0);
  EXPECT_GT(
      TimeTicksInSeconds(GetPaintTiming().FirstMeaningfulPaint()),
      TimeTicksInSeconds(GetPaintTiming().FirstMeaningfulPaintRendered()));
}

TEST_F(FirstMeaningfulPaintDetectorTest,
       WaitForMultipleOutstandingSwapPromisesAfterNetworkStable) {
  MarkFirstContentfulPaintAndClearSwapPromise();
  SimulateLayoutAndPaint(1);
  EXPECT_EQ(OutstandingDetectorSwapPromiseCount(), 1U);
  platform_->AdvanceClockSeconds(0.001);
  SimulateLayoutAndPaint(10);
  EXPECT_EQ(OutstandingDetectorSwapPromiseCount(), 2U);
  // Having outstanding swap promises should defer setting FMP.
  SimulateNetworkStable();
  EXPECT_EQ(TimeTicksInSeconds(GetPaintTiming().FirstMeaningfulPaintRendered()),
            0.0);
  EXPECT_EQ(TimeTicksInSeconds(GetPaintTiming().FirstMeaningfulPaint()), 0.0);
  // Clearing the first swap promise should have no effect on FMP.
  ClearProvisionalFirstMeaningfulPaintSwapPromise();
  EXPECT_EQ(OutstandingDetectorSwapPromiseCount(), 1U);
  EXPECT_EQ(TimeTicksInSeconds(GetPaintTiming().FirstMeaningfulPaintRendered()),
            0.0);
  EXPECT_EQ(TimeTicksInSeconds(GetPaintTiming().FirstMeaningfulPaint()), 0.0);
  double after_first_swap = AdvanceClockAndGetTime();
  // Clearing the last outstanding swap promise should set FMP.
  ClearProvisionalFirstMeaningfulPaintSwapPromise();
  EXPECT_EQ(OutstandingDetectorSwapPromiseCount(), 0U);
  EXPECT_GT(TimeTicksInSeconds(GetPaintTiming().FirstMeaningfulPaintRendered()),
            0.0);
  EXPECT_GT(TimeTicksInSeconds(GetPaintTiming().FirstMeaningfulPaint()), 0.0);
  EXPECT_GT(TimeTicksInSeconds(GetPaintTiming().FirstMeaningfulPaint()),
            after_first_swap);
  EXPECT_GT(
      TimeTicksInSeconds(GetPaintTiming().FirstMeaningfulPaint()),
      TimeTicksInSeconds(GetPaintTiming().FirstMeaningfulPaintRendered()));
}

TEST_F(FirstMeaningfulPaintDetectorTest,
       WaitForFirstContentfulPaintSwapAfterNetworkStable) {
  MarkFirstPaintAndClearSwapPromise();
  SimulateLayoutAndPaint(10);
  EXPECT_EQ(OutstandingDetectorSwapPromiseCount(), 1U);
  ClearProvisionalFirstMeaningfulPaintSwapPromise();
  double after_first_meaningful_paint_candidate = AdvanceClockAndGetTime();
  platform_->AdvanceClockSeconds(0.001);
  GetPaintTiming().MarkFirstContentfulPaint();
  // FCP > FMP candidate, but still waiting for FCP swap.
  SimulateNetworkStable();
  EXPECT_EQ(TimeTicksInSeconds(GetPaintTiming().FirstMeaningfulPaintRendered()),
            0.0);
  EXPECT_EQ(TimeTicksInSeconds(GetPaintTiming().FirstMeaningfulPaint()), 0.0);
  // Trigger notifying the detector about the FCP swap.
  ClearFirstContentfulPaintSwapPromise();
  EXPECT_GT(TimeTicksInSeconds(GetPaintTiming().FirstMeaningfulPaintRendered()),
            0.0);
  EXPECT_GT(TimeTicksInSeconds(GetPaintTiming().FirstMeaningfulPaint()), 0.0);
  EXPECT_EQ(
      TimeTicksInSeconds(GetPaintTiming().FirstMeaningfulPaintRendered()),
      TimeTicksInSeconds(GetPaintTiming().FirstContentfulPaintRendered()));
  EXPECT_EQ(TimeTicksInSeconds(GetPaintTiming().FirstMeaningfulPaint()),
            TimeTicksInSeconds(GetPaintTiming().FirstContentfulPaint()));
  EXPECT_GT(TimeTicksInSeconds(GetPaintTiming().FirstMeaningfulPaintRendered()),
            after_first_meaningful_paint_candidate);
  EXPECT_GT(
      TimeTicksInSeconds(GetPaintTiming().FirstMeaningfulPaint()),
      TimeTicksInSeconds(GetPaintTiming().FirstMeaningfulPaintRendered()));
}

TEST_F(FirstMeaningfulPaintDetectorTest,
       ProvisionalTimestampChangesAfterNetworkQuietWithOutstandingSwapPromise) {
  MarkFirstContentfulPaintAndClearSwapPromise();
  SimulateLayoutAndPaint(1);
  EXPECT_EQ(OutstandingDetectorSwapPromiseCount(), 1U);

  // Simulate only network 2-quiet so provisional FMP will be set on next
  // layout.
  double pre_stable_timestamp = AdvanceClockAndGetTime();
  platform_->AdvanceClockSeconds(0.001);
  SimulateNetwork2Quiet();
  EXPECT_EQ(TimeTicksInSeconds(GetPaintTiming().FirstMeaningfulPaintRendered()),
            0.0);
  EXPECT_EQ(TimeTicksInSeconds(GetPaintTiming().FirstMeaningfulPaint()), 0.0);

  // Force another FMP candidate while there is a pending swap promise and the
  // network 2-quiet FMP non-swap timestamp is set.
  platform_->AdvanceClockSeconds(0.001);
  SimulateLayoutAndPaint(10);
  EXPECT_EQ(OutstandingDetectorSwapPromiseCount(), 1U);

  // Simulate a delay in receiving the SwapPromise timestamp. Clearing this
  // SwapPromise will set FMP, and this will crash if the new provisional
  // non-swap timestamp is used.
  ClearProvisionalFirstMeaningfulPaintSwapPromise(pre_stable_timestamp);
  EXPECT_EQ(OutstandingDetectorSwapPromiseCount(), 0U);
  EXPECT_GT(TimeTicksInSeconds(GetPaintTiming().FirstMeaningfulPaintRendered()),
            0.0);
  EXPECT_GT(TimeTicksInSeconds(GetPaintTiming().FirstMeaningfulPaint()), 0.0);
  EXPECT_EQ(TimeTicksInSeconds(GetPaintTiming().FirstMeaningfulPaint()),
            pre_stable_timestamp);
  EXPECT_LT(TimeTicksInSeconds(GetPaintTiming().FirstMeaningfulPaintRendered()),
            pre_stable_timestamp);
}

}  // namespace blink
