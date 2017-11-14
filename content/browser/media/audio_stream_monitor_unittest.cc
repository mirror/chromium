// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/media/audio_stream_monitor.h"

#include <map>
#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/macros.h"
#include "base/test/simple_test_tick_clock.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "content/public/browser/invalidate_type.h"
#include "content/public/browser/web_contents_delegate.h"
#include "content/public/test/test_renderer_host.h"
#include "media/audio/audio_power_monitor.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::InvokeWithoutArgs;

namespace content {

namespace {

const int kRenderProcessId = 1;
const int kAnotherRenderProcessId = 2;
const int kStreamId = 3;
const int kAnotherStreamId = 6;
const int kRenderFrameId = 4;
const int kAnotherRenderFrameId = 8;

// Used to confirm audio indicator state changes occur at the correct times.
class MockWebContentsDelegate : public WebContentsDelegate {
 public:
  MOCK_METHOD2(NavigationStateChanged,
               void(WebContents* source, InvalidateTypes changed_flags));
};

}  // namespace

class AudioStreamMonitorTest : public RenderViewHostTestHarness {
 public:
  AudioStreamMonitorTest() {
    // Start |clock_| at non-zero.
    clock_.Advance(base::TimeDelta::FromSeconds(1000000));
  }

  void SetUp() override {
    RenderViewHostTestHarness::SetUp();

    WebContentsImpl* web_contents = reinterpret_cast<WebContentsImpl*>(
        RenderViewHostTestHarness::web_contents());
    web_contents->SetDelegate(&mock_web_contents_delegate_);

    monitor_ = web_contents->audio_stream_monitor();
    const_cast<base::TickClock*&>(monitor_->clock_) = &clock_;
  }

  base::TimeTicks GetTestClockTime() { return clock_.NowTicks(); }

  void AdvanceClock(const base::TimeDelta& delta) { clock_.Advance(delta); }

  void SimulateOffTimerFired() { monitor_->MaybeToggle(); }

  void ExpectIsMonitoring(int render_process_id,
                          int render_frame_id,
                          int stream_id,
                          bool is_polling) {
    const AudioStreamMonitor::StreamID key = {render_process_id,
                                              render_frame_id, stream_id};
    EXPECT_EQ(is_polling,
              monitor_->streams_.find(key) != monitor_->streams_.end());
    if (!audible_state_monitoring_available())
      EXPECT_EQ(monitor_->streams_.empty(), !monitor_->IsCurrentlyAudible());
  }

  void ExpectTabWasRecentlyAudible(
      bool was_audible,
      const base::TimeTicks& last_became_audible_time) {
    EXPECT_EQ(was_audible, monitor_->indicator_is_on_);
    EXPECT_EQ(last_became_audible_time, monitor_->last_became_audible_time_);
    EXPECT_EQ(monitor_->indicator_is_on_, monitor_->off_timer_.IsRunning());
  }

  void ExpectIsCurrentlyAudible() const {
    EXPECT_TRUE(monitor_->IsCurrentlyAudible());
  }

  void ExpectNotCurrentlyAudible() const {
    EXPECT_FALSE(monitor_->IsCurrentlyAudible());
  }

  void ExpectRecentlyAudibleChangeNotification(bool new_recently_audible) {
    EXPECT_CALL(
        mock_web_contents_delegate_,
        NavigationStateChanged(RenderViewHostTestHarness::web_contents(),
                               INVALIDATE_TYPE_TAB))
        .WillOnce(InvokeWithoutArgs(
            this,
            new_recently_audible
                ? &AudioStreamMonitorTest::ExpectWasRecentlyAudible
                : &AudioStreamMonitorTest::ExpectNotRecentlyAudible))
        .RetiresOnSaturation();
  }

  void ExpectCurrentlyAudibleChangeNotification(bool new_audible) {
    EXPECT_CALL(
        mock_web_contents_delegate_,
        NavigationStateChanged(RenderViewHostTestHarness::web_contents(),
                               INVALIDATE_TYPE_TAB))
        .WillOnce(InvokeWithoutArgs(
            this,
            new_audible
                ? &AudioStreamMonitorTest::ExpectIsCurrentlyAudible
                : &AudioStreamMonitorTest::ExpectNotCurrentlyAudible))
        .RetiresOnSaturation();
  }

  // A small time step useful for testing the passage of time.
  static base::TimeDelta one_time_step() {
    return base::TimeDelta::FromSeconds(1) / 15;
  }

  static base::TimeDelta holding_period() {
    return base::TimeDelta::FromMilliseconds(
        AudioStreamMonitor::kHoldOnMilliseconds);
  }

  void StartMonitoring(int render_process_id,
                       int render_frame_id,
                       int stream_id) {
    if (!audible_state_monitoring_available() && monitor_->streams_.empty()) {
      ExpectCurrentlyAudibleChangeNotification(true);
    }
    monitor_->StartMonitoringStreamOnUIThread(render_process_id,
                                              render_frame_id, stream_id);
  }

  void StopMonitoring(int render_process_id,
                      int render_frame_id,
                      int stream_id) {
    if (!audible_state_monitoring_available() &&
        monitor_->streams_.size() == 1u) {
      ExpectCurrentlyAudibleChangeNotification(false);
    }
    monitor_->StopMonitoringStreamOnUIThread(render_process_id, render_frame_id,
                                             stream_id);
  }

  void UpdateAudibleState(int render_process_id,
                          int render_frame_id,
                          int stream_id,
                          bool is_audible) {
    monitor_->UpdateStreamAudibleStateOnUIThread(
        render_process_id, render_frame_id, stream_id, is_audible);
  }

  bool audible_state_monitoring_available() {
    return AudioStreamMonitor::audible_state_monitoring_available();
  }

 protected:
  AudioStreamMonitor* monitor_;

 private:
  void ExpectWasRecentlyAudible() const {
    EXPECT_TRUE(monitor_->WasRecentlyAudible());
  }

  void ExpectNotRecentlyAudible() const {
    EXPECT_FALSE(monitor_->WasRecentlyAudible());
  }

  MockWebContentsDelegate mock_web_contents_delegate_;
  base::SimpleTestTickClock clock_;

  DISALLOW_COPY_AND_ASSIGN(AudioStreamMonitorTest);
};

TEST_F(AudioStreamMonitorTest, MonitorsWhenProvidedAStream) {
  if (!audible_state_monitoring_available())
    return;

  EXPECT_FALSE(monitor_->WasRecentlyAudible());
  ExpectNotCurrentlyAudible();
  ExpectIsMonitoring(kRenderProcessId, kRenderFrameId, kStreamId, false);

  StartMonitoring(kRenderProcessId, kRenderFrameId, kStreamId);
  EXPECT_FALSE(monitor_->WasRecentlyAudible());
  ExpectNotCurrentlyAudible();
  ExpectIsMonitoring(kRenderProcessId, kRenderFrameId, kStreamId, true);

  StopMonitoring(kRenderProcessId, kRenderFrameId, kStreamId);
  EXPECT_FALSE(monitor_->WasRecentlyAudible());
  ExpectNotCurrentlyAudible();
  ExpectIsMonitoring(kRenderProcessId, kRenderFrameId, kStreamId, false);
}

// Tests that AudioStreamMonitor keeps the indicator on for the holding period
// even if there is silence during the holding period.
// See comments in audio_stream_monitor.h for expected behavior.
TEST_F(AudioStreamMonitorTest, IndicatorIsOnUntilHoldingPeriodHasPassed) {
  if (!audible_state_monitoring_available())
    return;

  StartMonitoring(kRenderProcessId, kRenderFrameId, kStreamId);

  // Expect WebContents will get one call form AudioStreamMonitor to toggle the
  // indicator upon the very first notification that the stream has become
  // audible.
  ExpectRecentlyAudibleChangeNotification(true);

  // Loop, each time testing a slightly longer period of silence.  The recently
  // audible state should not change while the currently audible one should.
  int num_silence_steps = 1;
  base::TimeTicks last_became_audible_time;
  do {
    ExpectCurrentlyAudibleChangeNotification(true);

    UpdateAudibleState(kRenderProcessId, kRenderFrameId, kStreamId, true);
    last_became_audible_time = GetTestClockTime();
    ExpectTabWasRecentlyAudible(true, last_became_audible_time);
    AdvanceClock(one_time_step());

    ExpectCurrentlyAudibleChangeNotification(false);

    // Notify that the stream has become silent and advance time repeatedly,
    // ensuring that the indicator is being held on during the holding period.
    UpdateAudibleState(kRenderProcessId, kRenderFrameId, kStreamId, false);
    ExpectTabWasRecentlyAudible(true, last_became_audible_time);
    for (int i = 0; i < num_silence_steps; ++i) {
      // Note: Redundant off timer firings should not have any effect.
      SimulateOffTimerFired();
      ExpectTabWasRecentlyAudible(true, last_became_audible_time);
      AdvanceClock(one_time_step());
    }

    ++num_silence_steps;
  } while (GetTestClockTime() < last_became_audible_time + holding_period());

  // At this point, the clock has just advanced to beyond the holding period, so
  // the next firing of the off timer should turn off the tab indicator.  Also,
  // make sure it stays off for several cycles thereafter.
  ExpectRecentlyAudibleChangeNotification(false);
  for (int i = 0; i < 10; ++i) {
    SimulateOffTimerFired();
    ExpectTabWasRecentlyAudible(false, last_became_audible_time);
    AdvanceClock(one_time_step());
  }
}

// Tests that the AudioStreamMonitor correctly processes updates from two
// different streams in the same tab.
TEST_F(AudioStreamMonitorTest, HandlesMultipleStreamUpdate) {
  if (!audible_state_monitoring_available())
    return;

  StartMonitoring(kRenderProcessId, kRenderFrameId, kStreamId);
  StartMonitoring(kRenderProcessId, kAnotherRenderFrameId, kAnotherStreamId);

  base::TimeTicks last_became_audible_time;
  ExpectTabWasRecentlyAudible(false, last_became_audible_time);
  ExpectNotCurrentlyAudible();

  // Measure audible sound from the first stream and silence from the second.
  // The tab becomes audible.
  ExpectRecentlyAudibleChangeNotification(true);
  ExpectCurrentlyAudibleChangeNotification(true);

  UpdateAudibleState(kRenderProcessId, kRenderFrameId, kStreamId, true);
  UpdateAudibleState(kRenderProcessId, kAnotherRenderFrameId, kAnotherStreamId,
                     false);
  last_became_audible_time = GetTestClockTime();
  ExpectTabWasRecentlyAudible(true, last_became_audible_time);
  ExpectIsCurrentlyAudible();

  // Halfway through the holding period, the second stream joins in.  The
  // indicator stays on.
  AdvanceClock(holding_period() / 2);
  SimulateOffTimerFired();
  UpdateAudibleState(kRenderProcessId, kAnotherRenderFrameId, kAnotherStreamId,
                     true);
  ExpectTabWasRecentlyAudible(true, last_became_audible_time);
  ExpectIsCurrentlyAudible();

  // Now, measure silence from both streams.  After an entire holding period
  // has passed (since the second stream joined in), the tab will no longer
  // become audible nor recently audible.
  ExpectCurrentlyAudibleChangeNotification(false);
  ExpectRecentlyAudibleChangeNotification(false);

  AdvanceClock(holding_period() / 2);
  SimulateOffTimerFired();
  UpdateAudibleState(kRenderProcessId, kRenderFrameId, kStreamId, false);
  UpdateAudibleState(kRenderProcessId, kAnotherRenderFrameId, kAnotherStreamId,
                     false);
  SimulateOffTimerFired();
  ExpectTabWasRecentlyAudible(false, last_became_audible_time);
  ExpectNotCurrentlyAudible();

  // Now, measure silence from the first stream and audible sound from the
  // second.  The tab becomes audible again.
  ExpectRecentlyAudibleChangeNotification(true);
  ExpectCurrentlyAudibleChangeNotification(true);

  UpdateAudibleState(kRenderProcessId, kAnotherRenderFrameId, kAnotherStreamId,
                     true);
  last_became_audible_time = GetTestClockTime();
  ExpectTabWasRecentlyAudible(true, last_became_audible_time);
  ExpectIsCurrentlyAudible();

  // From here onwards, both streams are silent.  Halfway through the holding
  // period, the tab is no longer audible but stays as recently audible.
  ExpectCurrentlyAudibleChangeNotification(false);

  UpdateAudibleState(kRenderProcessId, kAnotherRenderFrameId, kAnotherStreamId,
                     false);
  AdvanceClock(holding_period() / 2);
  SimulateOffTimerFired();
  ExpectTabWasRecentlyAudible(true, last_became_audible_time);
  ExpectNotCurrentlyAudible();

  // Just past the holding period, the tab is no longer marked as recently
  // audible.
  ExpectRecentlyAudibleChangeNotification(false);

  AdvanceClock(holding_period() -
               (GetTestClockTime() - last_became_audible_time));
  SimulateOffTimerFired();
  ExpectTabWasRecentlyAudible(false, last_became_audible_time);
  ExpectNotCurrentlyAudible();

  // The passage of time should not turn the indicator back while both streams
  // are remaining silent.
  for (int i = 0; i < 100; ++i) {
    AdvanceClock(one_time_step());
    ExpectTabWasRecentlyAudible(false, last_became_audible_time);
    ExpectNotCurrentlyAudible();
  }
}

TEST_F(AudioStreamMonitorTest, MultipleRendererProcesses) {
  StartMonitoring(kRenderProcessId, kRenderFrameId, kStreamId);
  StartMonitoring(kAnotherRenderProcessId, kRenderFrameId, kStreamId);
  ExpectIsMonitoring(kRenderProcessId, kRenderFrameId, kStreamId, true);
  ExpectIsMonitoring(kAnotherRenderProcessId, kRenderFrameId, kStreamId, true);
  StopMonitoring(kAnotherRenderProcessId, kRenderFrameId, kStreamId);
  ExpectIsMonitoring(kRenderProcessId, kRenderFrameId, kStreamId, true);
  ExpectIsMonitoring(kAnotherRenderProcessId, kRenderFrameId, kStreamId, false);
}

TEST_F(AudioStreamMonitorTest, RenderProcessGone) {
  StartMonitoring(kRenderProcessId, kRenderFrameId, kStreamId);
  StartMonitoring(kAnotherRenderProcessId, kRenderFrameId, kStreamId);
  ExpectIsMonitoring(kRenderProcessId, kRenderFrameId, kStreamId, true);
  ExpectIsMonitoring(kAnotherRenderProcessId, kRenderFrameId, kStreamId, true);
  monitor_->RenderProcessGone(kRenderProcessId);
  ExpectIsMonitoring(kRenderProcessId, kRenderFrameId, kStreamId, false);
  if (!audible_state_monitoring_available())
    ExpectCurrentlyAudibleChangeNotification(false);
  monitor_->RenderProcessGone(kAnotherRenderProcessId);
  ExpectIsMonitoring(kAnotherRenderProcessId, kRenderFrameId, kStreamId, false);
}

TEST_F(AudioStreamMonitorTest, NoPowerLevelMonitoring) {
  if (audible_state_monitoring_available())
    return;

  ExpectNotCurrentlyAudible();
  StartMonitoring(kRenderProcessId, kRenderFrameId, kStreamId);
  ExpectIsCurrentlyAudible();
  ExpectIsMonitoring(kRenderProcessId, kRenderFrameId, kStreamId, true);

  StartMonitoring(kAnotherRenderProcessId, kRenderFrameId, kStreamId);
  ExpectIsCurrentlyAudible();
  ExpectIsMonitoring(kAnotherRenderProcessId, kRenderFrameId, kStreamId, true);

  StopMonitoring(kRenderProcessId, kRenderFrameId, kStreamId);
  ExpectIsCurrentlyAudible();
  StopMonitoring(kAnotherRenderProcessId, kRenderFrameId, kStreamId);
  ExpectNotCurrentlyAudible();
}

}  // namespace content
