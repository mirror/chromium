// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/scheduler/compositor_timing_history.h"

#include <chrono>
#include <thread>
#include "base/macros.h"
#include "base/test/histogram_tester.h"
#include "cc/debug/rendering_stats_instrumentation.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace cc {
namespace {

class CompositorTimingHistoryTest;

class TestCompositorTimingHistory : public CompositorTimingHistory {
 public:
  TestCompositorTimingHistory(CompositorTimingHistoryTest* test,
                              RenderingStatsInstrumentation* rendering_stats)
      : CompositorTimingHistory(false, RENDERER_UMA, rendering_stats),
        test_(test) {}
  // To avoid hitting the DCHECK in CompositorTimingHistory::DidDraw().
  void SetDrawTestTime() { draw_start_time_ = Now(); }
  void SetActiveTreeMainFrameTime() { active_tree_main_frame_time_ = Now(); }
  base::TimeDelta MainThreadAnimationsDrawInterval() {
    return main_thread_animations_draw_interval_for_testing_;
  }
  base::TimeDelta MainThreadCompositableAnimationsDrawInterval() {
    return main_thread_compositable_animations_draw_interval_for_testing_;
  }
  base::TimeDelta CompositedAnimationsDrawInterval() {
    return composited_animations_draw_interval_for_testing_;
  }

 protected:
  base::TimeTicks Now() const override;

  CompositorTimingHistoryTest* test_;

 private:
  DISALLOW_COPY_AND_ASSIGN(TestCompositorTimingHistory);
};

class CompositorTimingHistoryTest : public testing::Test {
 public:
  CompositorTimingHistoryTest()
      : rendering_stats_(RenderingStatsInstrumentation::Create()),
        timing_history_(this, rendering_stats_.get()) {
    AdvanceNowBy(base::TimeDelta::FromMilliseconds(1));
    timing_history_.SetRecordingEnabled(true);
  }

  void AdvanceNowBy(base::TimeDelta delta) { now_ += delta; }

  base::TimeTicks Now() { return now_; }

 protected:
  std::unique_ptr<RenderingStatsInstrumentation> rendering_stats_;
  TestCompositorTimingHistory timing_history_;
  base::TimeTicks now_;
};

base::TimeTicks TestCompositorTimingHistory::Now() const {
  return test_->Now();
}

TEST_F(CompositorTimingHistoryTest, AllSequential_Commit) {
  base::TimeDelta one_second = base::TimeDelta::FromSeconds(1);

  // Critical BeginMainFrames are faster than non critical ones,
  // as expected.
  base::TimeDelta begin_main_frame_queue_duration =
      base::TimeDelta::FromMilliseconds(1);
  base::TimeDelta begin_main_frame_start_to_commit_duration =
      base::TimeDelta::FromMilliseconds(1);
  base::TimeDelta prepare_tiles_duration = base::TimeDelta::FromMilliseconds(2);
  base::TimeDelta prepare_tiles_end_to_ready_to_activate_duration =
      base::TimeDelta::FromMilliseconds(1);
  base::TimeDelta commit_to_ready_to_activate_duration =
      base::TimeDelta::FromMilliseconds(3);
  base::TimeDelta activate_duration = base::TimeDelta::FromMilliseconds(4);
  base::TimeDelta draw_duration = base::TimeDelta::FromMilliseconds(5);

  timing_history_.WillBeginMainFrame(true, Now());
  AdvanceNowBy(begin_main_frame_queue_duration);
  timing_history_.BeginMainFrameStarted(Now());
  AdvanceNowBy(begin_main_frame_start_to_commit_duration);
  timing_history_.DidCommit();
  timing_history_.WillPrepareTiles();
  AdvanceNowBy(prepare_tiles_duration);
  timing_history_.DidPrepareTiles();
  AdvanceNowBy(prepare_tiles_end_to_ready_to_activate_duration);
  timing_history_.ReadyToActivate();
  // Do not count idle time between notification and actual activation.
  AdvanceNowBy(one_second);
  timing_history_.WillActivate();
  AdvanceNowBy(activate_duration);
  timing_history_.DidActivate();
  // Do not count idle time between activate and draw.
  AdvanceNowBy(one_second);
  timing_history_.WillDraw();
  AdvanceNowBy(draw_duration);
  timing_history_.DidDraw(true, true, Now(), 0, 0, 0);

  EXPECT_EQ(begin_main_frame_queue_duration,
            timing_history_.BeginMainFrameQueueDurationCriticalEstimate());
  EXPECT_EQ(begin_main_frame_queue_duration,
            timing_history_.BeginMainFrameQueueDurationNotCriticalEstimate());

  EXPECT_EQ(begin_main_frame_start_to_commit_duration,
            timing_history_.BeginMainFrameStartToCommitDurationEstimate());

  EXPECT_EQ(commit_to_ready_to_activate_duration,
            timing_history_.CommitToReadyToActivateDurationEstimate());
  EXPECT_EQ(prepare_tiles_duration,
            timing_history_.PrepareTilesDurationEstimate());
  EXPECT_EQ(activate_duration, timing_history_.ActivateDurationEstimate());
  EXPECT_EQ(draw_duration, timing_history_.DrawDurationEstimate());
}

TEST_F(CompositorTimingHistoryTest, AllSequential_BeginMainFrameAborted) {
  base::TimeDelta one_second = base::TimeDelta::FromSeconds(1);

  base::TimeDelta begin_main_frame_queue_duration =
      base::TimeDelta::FromMilliseconds(1);
  base::TimeDelta begin_main_frame_start_to_commit_duration =
      base::TimeDelta::FromMilliseconds(1);
  base::TimeDelta prepare_tiles_duration = base::TimeDelta::FromMilliseconds(2);
  base::TimeDelta prepare_tiles_end_to_ready_to_activate_duration =
      base::TimeDelta::FromMilliseconds(1);
  base::TimeDelta activate_duration = base::TimeDelta::FromMilliseconds(4);
  base::TimeDelta draw_duration = base::TimeDelta::FromMilliseconds(5);

  timing_history_.WillBeginMainFrame(false, Now());
  AdvanceNowBy(begin_main_frame_queue_duration);
  timing_history_.BeginMainFrameStarted(Now());
  AdvanceNowBy(begin_main_frame_start_to_commit_duration);
  // BeginMainFrameAborted counts as a commit complete.
  timing_history_.BeginMainFrameAborted();
  timing_history_.WillPrepareTiles();
  AdvanceNowBy(prepare_tiles_duration);
  timing_history_.DidPrepareTiles();
  AdvanceNowBy(prepare_tiles_end_to_ready_to_activate_duration);
  // Do not count idle time between notification and actual activation.
  AdvanceNowBy(one_second);
  timing_history_.WillActivate();
  AdvanceNowBy(activate_duration);
  timing_history_.DidActivate();
  // Do not count idle time between activate and draw.
  AdvanceNowBy(one_second);
  timing_history_.WillDraw();
  AdvanceNowBy(draw_duration);
  timing_history_.DidDraw(false, false, Now(), 0, 0, 0);

  EXPECT_EQ(base::TimeDelta(),
            timing_history_.BeginMainFrameQueueDurationCriticalEstimate());
  EXPECT_EQ(begin_main_frame_queue_duration,
            timing_history_.BeginMainFrameQueueDurationNotCriticalEstimate());

  EXPECT_EQ(begin_main_frame_start_to_commit_duration,
            timing_history_.BeginMainFrameStartToCommitDurationEstimate());

  EXPECT_EQ(prepare_tiles_duration,
            timing_history_.PrepareTilesDurationEstimate());
  EXPECT_EQ(activate_duration, timing_history_.ActivateDurationEstimate());
  EXPECT_EQ(draw_duration, timing_history_.DrawDurationEstimate());
}

TEST_F(CompositorTimingHistoryTest, BeginMainFrame_CriticalFaster) {
  // Critical BeginMainFrames are faster than non critical ones.
  base::TimeDelta begin_main_frame_queue_duration_critical =
      base::TimeDelta::FromMilliseconds(1);
  base::TimeDelta begin_main_frame_queue_duration_not_critical =
      base::TimeDelta::FromMilliseconds(2);
  base::TimeDelta begin_main_frame_start_to_commit_duration =
      base::TimeDelta::FromMilliseconds(1);

  timing_history_.WillBeginMainFrame(true, Now());
  AdvanceNowBy(begin_main_frame_queue_duration_critical);
  timing_history_.BeginMainFrameStarted(Now());
  AdvanceNowBy(begin_main_frame_start_to_commit_duration);
  timing_history_.BeginMainFrameAborted();

  timing_history_.WillBeginMainFrame(false, Now());
  AdvanceNowBy(begin_main_frame_queue_duration_not_critical);
  timing_history_.BeginMainFrameStarted(Now());
  AdvanceNowBy(begin_main_frame_start_to_commit_duration);
  timing_history_.BeginMainFrameAborted();

  // Since the critical BeginMainFrames are faster than non critical ones,
  // the expectations are straightforward.
  EXPECT_EQ(begin_main_frame_queue_duration_critical,
            timing_history_.BeginMainFrameQueueDurationCriticalEstimate());
  EXPECT_EQ(begin_main_frame_queue_duration_not_critical,
            timing_history_.BeginMainFrameQueueDurationNotCriticalEstimate());
  EXPECT_EQ(begin_main_frame_start_to_commit_duration,
            timing_history_.BeginMainFrameStartToCommitDurationEstimate());
}

TEST_F(CompositorTimingHistoryTest, BeginMainFrames_OldCriticalSlower) {
  // Critical BeginMainFrames are slower than non critical ones,
  // which is unexpected, but could occur if one type of frame
  // hasn't been sent for a significant amount of time.
  base::TimeDelta begin_main_frame_queue_duration_critical =
      base::TimeDelta::FromMilliseconds(2);
  base::TimeDelta begin_main_frame_queue_duration_not_critical =
      base::TimeDelta::FromMilliseconds(1);
  base::TimeDelta begin_main_frame_start_to_commit_duration =
      base::TimeDelta::FromMilliseconds(1);

  // A single critical frame that is slow.
  timing_history_.WillBeginMainFrame(true, Now());
  AdvanceNowBy(begin_main_frame_queue_duration_critical);
  timing_history_.BeginMainFrameStarted(Now());
  AdvanceNowBy(begin_main_frame_start_to_commit_duration);
  // BeginMainFrameAborted counts as a commit complete.
  timing_history_.BeginMainFrameAborted();

  // A bunch of faster non critical frames that are newer.
  for (int i = 0; i < 100; i++) {
    timing_history_.WillBeginMainFrame(false, Now());
    AdvanceNowBy(begin_main_frame_queue_duration_not_critical);
    timing_history_.BeginMainFrameStarted(Now());
    AdvanceNowBy(begin_main_frame_start_to_commit_duration);
    // BeginMainFrameAborted counts as a commit complete.
    timing_history_.BeginMainFrameAborted();
  }

  // Recent fast non critical BeginMainFrames should result in the
  // critical estimate also being fast.
  EXPECT_EQ(begin_main_frame_queue_duration_not_critical,
            timing_history_.BeginMainFrameQueueDurationCriticalEstimate());
  EXPECT_EQ(begin_main_frame_queue_duration_not_critical,
            timing_history_.BeginMainFrameQueueDurationNotCriticalEstimate());

  EXPECT_EQ(begin_main_frame_start_to_commit_duration,
            timing_history_.BeginMainFrameStartToCommitDurationEstimate());
}

TEST_F(CompositorTimingHistoryTest, BeginMainFrames_NewCriticalSlower) {
  // Critical BeginMainFrames are slower than non critical ones,
  // which is unexpected, but could occur if one type of frame
  // hasn't been sent for a significant amount of time.
  base::TimeDelta begin_main_frame_queue_duration_critical =
      base::TimeDelta::FromMilliseconds(2);
  base::TimeDelta begin_main_frame_queue_duration_not_critical =
      base::TimeDelta::FromMilliseconds(1);
  base::TimeDelta begin_main_frame_start_to_commit_duration =
      base::TimeDelta::FromMilliseconds(1);

  // A single non critical frame that is fast.
  timing_history_.WillBeginMainFrame(false, Now());
  AdvanceNowBy(begin_main_frame_queue_duration_not_critical);
  timing_history_.BeginMainFrameStarted(Now());
  AdvanceNowBy(begin_main_frame_start_to_commit_duration);
  timing_history_.BeginMainFrameAborted();

  // A bunch of slower critical frames that are newer.
  for (int i = 0; i < 100; i++) {
    timing_history_.WillBeginMainFrame(true, Now());
    AdvanceNowBy(begin_main_frame_queue_duration_critical);
    timing_history_.BeginMainFrameStarted(Now());
    AdvanceNowBy(begin_main_frame_start_to_commit_duration);
    timing_history_.BeginMainFrameAborted();
  }

  // Recent slow critical BeginMainFrames should result in the
  // not critical estimate also being slow.
  EXPECT_EQ(begin_main_frame_queue_duration_critical,
            timing_history_.BeginMainFrameQueueDurationCriticalEstimate());
  EXPECT_EQ(begin_main_frame_queue_duration_critical,
            timing_history_.BeginMainFrameQueueDurationNotCriticalEstimate());

  EXPECT_EQ(begin_main_frame_start_to_commit_duration,
            timing_history_.BeginMainFrameStartToCommitDurationEstimate());
}

void TestAnimationUMA(const base::HistogramTester& histogram_tester,
                      base::HistogramBase::Count expected_count1,
                      base::HistogramBase::Count expected_count2,
                      base::HistogramBase::Count expected_count3) {
  histogram_tester.ExpectTotalCount(
      "Scheduling.Renderer.DrawIntervalWithCompositedAnimations2",
      expected_count1);
  histogram_tester.ExpectTotalCount(
      "Scheduling.Renderer.DrawIntervalWithMainThreadAnimations2",
      expected_count2);
  histogram_tester.ExpectTotalCount(
      "Scheduling.Renderer.DrawIntervalWithMainThreadCompositableAnimations2",
      expected_count3);
}

TEST_F(CompositorTimingHistoryTest, AnimationUMA) {
  base::HistogramTester histogram_tester;
  timing_history_.WillBeginMainFrame(true, Now());
  timing_history_.DidCommit();

  timing_history_.SetDrawTestTime();
  timing_history_.SetActiveTreeMainFrameTime();
  timing_history_.DidDraw(true, true, Now(), 0, 0, 0);
  TestAnimationUMA(histogram_tester, 0, 0, 0);

  timing_history_.SetDrawTestTime();
  timing_history_.SetActiveTreeMainFrameTime();
  timing_history_.DidDraw(true, true, Now(), 1, 1, 0);
  // Previous frame had no animation, so won't report anything in this frame.
  TestAnimationUMA(histogram_tester, 0, 0, 0);

  timing_history_.SetDrawTestTime();
  timing_history_.SetActiveTreeMainFrameTime();
  AdvanceNowBy(base::TimeDelta::FromMicroseconds(230));
  timing_history_.DidDraw(true, true, Now(), 0, 1, 0);
  // Report the main thread animation since the previous frame had one.
  TestAnimationUMA(histogram_tester, 0, 1, 0);
  EXPECT_EQ(timing_history_.MainThreadAnimationsDrawInterval(),
            base::TimeDelta::FromMicroseconds(230));

  timing_history_.SetDrawTestTime();
  timing_history_.SetActiveTreeMainFrameTime();
  AdvanceNowBy(base::TimeDelta::FromMicroseconds(145));
  timing_history_.DidDraw(true, true, Now(), 1, 0, 1);
  // Composited animation won't be reported because previous frame doesn't have
  // any composited animation.
  TestAnimationUMA(histogram_tester, 0, 1, 1);
  // This frame has no main thread animation, we won't report it which means
  // that the draw_interval remains the unchanged from the previous frame.
  EXPECT_EQ(timing_history_.MainThreadAnimationsDrawInterval(),
            base::TimeDelta::FromMicroseconds(230));
  EXPECT_EQ(timing_history_.MainThreadCompositableAnimationsDrawInterval(),
            base::TimeDelta::FromMicroseconds(145));

  timing_history_.SetDrawTestTime();
  timing_history_.SetActiveTreeMainFrameTime();
  AdvanceNowBy(base::TimeDelta::FromMicroseconds(456));
  timing_history_.DidDraw(true, true, Now(), 1, 1, 0);
  // Start reporting the composited animation.
  TestAnimationUMA(histogram_tester, 1, 2, 1);
  EXPECT_EQ(timing_history_.CompositedAnimationsDrawInterval(),
            base::TimeDelta::FromMicroseconds(456));
  // In the previous frame, we do have a main thread compositable
  // animation, so we are reporting the main thread animation in this frame.
  EXPECT_EQ(timing_history_.MainThreadAnimationsDrawInterval(),
            base::TimeDelta::FromMicroseconds(456));
  // No main thread compositable animation in this frame, so the draw
  // interval remains unchanged from the previous frame.
  EXPECT_EQ(timing_history_.MainThreadCompositableAnimationsDrawInterval(),
            base::TimeDelta::FromMicroseconds(145));

  timing_history_.SetDrawTestTime();
  timing_history_.SetActiveTreeMainFrameTime();
  AdvanceNowBy(base::TimeDelta::FromMicroseconds(321));
  timing_history_.DidDraw(true, true, Now(), 1, 1, 1);
  TestAnimationUMA(histogram_tester, 2, 3, 2);
  EXPECT_EQ(timing_history_.CompositedAnimationsDrawInterval(),
            base::TimeDelta::FromMicroseconds(321));
  EXPECT_EQ(timing_history_.MainThreadAnimationsDrawInterval(),
            base::TimeDelta::FromMicroseconds(321));
  EXPECT_EQ(timing_history_.MainThreadCompositableAnimationsDrawInterval(),
            base::TimeDelta::FromMicroseconds(321));
}

}  // namespace
}  // namespace cc
