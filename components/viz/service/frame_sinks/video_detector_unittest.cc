// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include <set>

#include "base/compiler_specific.h"
#include "base/containers/circular_deque.h"
#include "base/time/time.h"
#include "components/viz/common/quads/surface_draw_quad.h"
#include "components/viz/common/surfaces/local_surface_id_allocator.h"
#include "components/viz/service/display/surface_aggregator.h"
#include "components/viz/service/frame_sinks/compositor_frame_sink_support.h"
#include "components/viz/service/frame_sinks/frame_sink_manager_impl.h"
#include "components/viz/service/frame_sinks/video_detector.h"
#include "components/viz/test/compositor_frame_helpers.cc"
#include "components/viz/test/fake_compositor_frame_sink_client.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/viz/public/interfaces/compositing/video_detector_client.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/gfx/geometry/rect.h"

namespace viz {

// Implementation that just records video state changes.
class TestObserver : public mojom::VideoDetectorClient {
 public:
  TestObserver() : binding_(this) {}

  void Bind(mojom::VideoDetectorClientRequest request) {
    binding_.Bind(std::move(request));
  }

  bool IsEmpty() {
    binding_.FlushForTesting();
    return states_.empty();
  }

  void Reset() {
    binding_.FlushForTesting();
    states_.clear();
  }

  // Pops and returns the earliest-received state.
  uint8_t PopState() {
    binding_.FlushForTesting();
    CHECK(!states_.empty());
    uint8_t first_state = states_.front();
    states_.pop_front();
    return first_state;
  }

  // mojom::VideoDetectorClient implementation.
  void OnVideoActivityStarted() override { states_.push_back(1); }
  void OnVideoActivityEnded() override { states_.push_back(0); }

 private:
  // States in the order they were received.
  base::circular_deque<uint8_t> states_;

  mojo::Binding<mojom::VideoDetectorClient> binding_;

  DISALLOW_COPY_AND_ASSIGN(TestObserver);
};

class VideoDetectorTest : public testing::Test {
 public:
  VideoDetectorTest()
      : kMinFps(VideoDetector::kMinFramesPerSecond),
        kMinRect(gfx::Point(0, 0),
                 gfx::Size(VideoDetector::kMinUpdateWidth,
                           VideoDetector::kMinUpdateHeight)),
        kMinDuration(base::TimeDelta::FromMilliseconds(
            VideoDetector::kMinVideoDurationMs)),
        kTimeout(
            base::TimeDelta::FromMilliseconds(VideoDetector::kVideoTimeoutMs)),
        surface_aggregator_(frame_sink_manager_.surface_manager(),
                            nullptr,
                            false) {}
  ~VideoDetectorTest() override {}

  void SetUp() override {
    mojom::VideoDetectorClientPtr video_detector_client;
    observer_.Bind(mojo::MakeRequest(&video_detector_client));
    frame_sink_manager_.AddVideoDetectorClient(
        std::move(video_detector_client));

    detector_ = frame_sink_manager_.video_detector_for_testing();

    now_ = base::TimeTicks::Now();
    detector_->set_now_for_test(now_);

    root_frame_sink_ = CreateFrameSink();
    root_frame_sink_->SubmitCompositorFrame(
        local_surface_id_allocator_.GenerateId(), test::MakeCompositorFrame());
  }

  void TearDown() override {}

 protected:
  // Move |detector_|'s idea of the current time forward by |delta|.
  void AdvanceTime(base::TimeDelta delta) {
    now_ += delta;
    detector_->set_now_for_test(now_);
  }

  void CreateDisplayFrame() {
    surface_aggregator_.Aggregate(root_frame_sink_->current_surface_id());
  }

  void EmbedClient(CompositorFrameSinkSupport* frame_sink) {
    embedded_clients_.insert(frame_sink);
    SubmitRootFrame();
  }

  void UnembedClient(CompositorFrameSinkSupport* frame_sink) {
    embedded_clients_.erase(frame_sink);
    SubmitRootFrame();
  }

  void SubmitRootFrame() {
    CompositorFrame frame = test::MakeCompositorFrame();
    RenderPass* render_pass = frame.render_pass_list.back().get();
    SharedQuadState* shared_quad_state =
        render_pass->CreateAndAppendSharedQuadState();
    for (CompositorFrameSinkSupport* frame_sink : embedded_clients_) {
      SurfaceDrawQuad* quad =
          render_pass->CreateAndAppendDrawQuad<SurfaceDrawQuad>();
      quad->SetNew(shared_quad_state, gfx::Rect(0, 0, 10, 10),
                   gfx::Rect(0, 0, 5, 5), frame_sink->current_surface_id(),
                   base::nullopt, SK_ColorMAGENTA);
    }
    root_frame_sink_->SubmitCompositorFrame(
        root_frame_sink_->local_surface_id(), std::move(frame));
  }

  void SendUpdate(CompositorFrameSinkSupport* frame_sink,
                  const gfx::Rect& damage) {
    LocalSurfaceId local_surface_id =
        frame_sink->local_surface_id().is_valid()
            ? frame_sink->local_surface_id()
            : local_surface_id_allocator_.GenerateId();
    frame_sink->SubmitCompositorFrame(local_surface_id,
                                      MakeDamagedCompositorFrame(damage));
  }

  // Report updates to |window| of area |region| at a rate of
  // |updates_per_second| over |duration|. The first update will be sent
  // immediately and |now_| will incremented by |duration| upon returning.
  void SendUpdates(CompositorFrameSinkSupport* frame_sink,
                   const gfx::Rect& damage,
                   int updates_per_second,
                   base::TimeDelta duration) {
    const base::TimeDelta time_between_updates =
        base::TimeDelta::FromSecondsD(1.0 / updates_per_second);
    const base::TimeTicks end_time = now_ + duration;
    while (now_ < end_time) {
      SendUpdate(frame_sink, damage);
      AdvanceTime(time_between_updates);
      CreateDisplayFrame();
    }
    now_ = end_time;
    detector_->set_now_for_test(now_);
  }

  std::unique_ptr<CompositorFrameSinkSupport> CreateFrameSink() {
    constexpr bool is_root = false;
    constexpr bool needs_sync_points = true;
    static uint32_t client_id = 1;
    FrameSinkId frame_sink_id(client_id++, 0);
    std::unique_ptr<CompositorFrameSinkSupport> frame_sink =
        CompositorFrameSinkSupport::Create(&frame_sink_client_,
                                           &frame_sink_manager_, frame_sink_id,
                                           is_root, needs_sync_points);
    SendUpdate(frame_sink.get(), gfx::Rect());
    return frame_sink;
  }

  // Constants placed here for convenience.
  const int kMinFps;
  const gfx::Rect kMinRect;
  const base::TimeDelta kMinDuration;
  const base::TimeDelta kTimeout;

  VideoDetector* detector_;  // not owned
  TestObserver observer_;

  // The current (fake) time used by |detector_|.
  base::TimeTicks now_;

 private:
  CompositorFrame MakeDamagedCompositorFrame(const gfx::Rect& damage) {
    CompositorFrame frame = test::MakeCompositorFrame(kFrameSinkSize);
    frame.render_pass_list.back()->damage_rect = damage;
    return frame;
  }

  const gfx::Size kFrameSinkSize = gfx::Size(10000, 10000);
  FrameSinkManagerImpl frame_sink_manager_;
  FakeCompositorFrameSinkClient frame_sink_client_;
  LocalSurfaceIdAllocator local_surface_id_allocator_;
  SurfaceAggregator surface_aggregator_;
  std::unique_ptr<CompositorFrameSinkSupport> root_frame_sink_;
  std::set<CompositorFrameSinkSupport*> embedded_clients_;

  DISALLOW_COPY_AND_ASSIGN(VideoDetectorTest);
};

TEST_F(VideoDetectorTest, DontReportWhenRegionTooSmall) {
  std::unique_ptr<CompositorFrameSinkSupport> frame_sink = CreateFrameSink();
  EmbedClient(frame_sink.get());
  gfx::Rect rect = kMinRect;
  rect.Inset(0, 0, 1, 0);
  SendUpdates(frame_sink.get(), rect, 2 * kMinFps, 2 * kMinDuration);
  EXPECT_TRUE(observer_.IsEmpty());
}

TEST_F(VideoDetectorTest, DontReportWhenFramerateTooLow) {
  std::unique_ptr<CompositorFrameSinkSupport> frame_sink = CreateFrameSink();
  EmbedClient(frame_sink.get());
  SendUpdates(frame_sink.get(), kMinRect, kMinFps - 5, 2 * kMinDuration);
  EXPECT_TRUE(observer_.IsEmpty());
}

TEST_F(VideoDetectorTest, DontReportWhenNotPlayingLongEnough) {
  std::unique_ptr<CompositorFrameSinkSupport> frame_sink = CreateFrameSink();
  EmbedClient(frame_sink.get());
  SendUpdates(frame_sink.get(), kMinRect, 2 * kMinFps, 0.5 * kMinDuration);
  EXPECT_TRUE(observer_.IsEmpty());

  SendUpdates(frame_sink.get(), kMinRect, 2 * kMinFps, 0.6 * kMinDuration);
  EXPECT_EQ(1u, observer_.PopState());
  EXPECT_TRUE(observer_.IsEmpty());
}

TEST_F(VideoDetectorTest, ReportStartAndStop) {
  const base::TimeDelta kDuration =
      kMinDuration + base::TimeDelta::FromMilliseconds(100);
  std::unique_ptr<CompositorFrameSinkSupport> frame_sink = CreateFrameSink();
  EmbedClient(frame_sink.get());
  SendUpdates(frame_sink.get(), kMinRect, kMinFps + 5, kDuration);
  EXPECT_EQ(1u, observer_.PopState());
  EXPECT_TRUE(observer_.IsEmpty());
  AdvanceTime(kTimeout);
  EXPECT_TRUE(detector_->TriggerTimeoutForTesting());
  EXPECT_EQ(0u, observer_.PopState());
  EXPECT_TRUE(observer_.IsEmpty());

  // The timer shouldn't be running anymore.
  EXPECT_FALSE(detector_->TriggerTimeoutForTesting());

  // Start playing again.
  SendUpdates(frame_sink.get(), kMinRect, kMinFps + 5, kDuration);
  EXPECT_EQ(1u, observer_.PopState());
  EXPECT_TRUE(observer_.IsEmpty());

  AdvanceTime(kTimeout);
  EXPECT_TRUE(detector_->TriggerTimeoutForTesting());
  EXPECT_EQ(0u, observer_.PopState());
  EXPECT_TRUE(observer_.IsEmpty());
}

TEST_F(VideoDetectorTest, ReportOnceForMultipleWindows) {
  gfx::Rect kWindowBounds(gfx::Point(), gfx::Size(1024, 768));
  std::unique_ptr<CompositorFrameSinkSupport> frame_sink1 = CreateFrameSink();
  std::unique_ptr<CompositorFrameSinkSupport> frame_sink2 = CreateFrameSink();
  EmbedClient(frame_sink1.get());
  EmbedClient(frame_sink2.get());

  // Even if there's video playing in both windows, the observer should only
  // receive a single notification.
  const int fps = 2 * kMinFps;
  const base::TimeDelta time_between_updates =
      base::TimeDelta::FromSecondsD(1.0 / fps);
  const base::TimeTicks start_time = now_;
  while (now_ < start_time + 2 * kMinDuration) {
    SendUpdate(frame_sink1.get(), kMinRect);
    SendUpdate(frame_sink2.get(), kMinRect);
    AdvanceTime(time_between_updates);
    CreateDisplayFrame();
  }
  EXPECT_EQ(1u, observer_.PopState());
  EXPECT_TRUE(observer_.IsEmpty());
}

TEST_F(VideoDetectorTest, DontReportWhenWindowHidden) {
  std::unique_ptr<CompositorFrameSinkSupport> frame_sink = CreateFrameSink();

  SendUpdates(frame_sink.get(), kMinRect, kMinFps + 5, 2 * kMinDuration);
  EXPECT_TRUE(observer_.IsEmpty());

  // Make the window visible.
  observer_.Reset();
  AdvanceTime(kTimeout);
  EmbedClient(frame_sink.get());
  SendUpdates(frame_sink.get(), kMinRect, kMinFps + 5, 2 * kMinDuration);
  EXPECT_EQ(1u, observer_.PopState());
  EXPECT_TRUE(observer_.IsEmpty());
}

/* Will be deleted

TEST_F(VideoDetectorTest, DontReportWhenWindowOffscreen) {
  std::unique_ptr<aura::Window> window =
      CreateTestWindow(gfx::Rect(0, 0, 1024, 768));
  window->SetBounds(
      gfx::Rect(gfx::Point(Shell::GetPrimaryRootWindow()->bounds().width(), 0),
                window->bounds().size()));
  SendUpdates(window.get(), kMinRect, 2 * kMinFps, 2 * kMinDuration);
  EXPECT_TRUE(observer_->IsEmpty());

  // Move the window onscreen.
  window->SetBounds(gfx::Rect(gfx::Point(0, 0), window->bounds().size()));
  SendUpdates(window.get(), kMinRect, 2 * kMinFps, 2 * kMinDuration);
  EXPECT_EQ(VideoDetector::State::PLAYING_WINDOWED, observer_->PopState());
  EXPECT_TRUE(observer_->IsEmpty());
}


*/

/*
 * Move this unit tests
TEST_F(VideoDetectorTest, DontReportDuringShutdown) {
  std::unique_ptr<aura::Window> window =
      CreateTestWindow(gfx::Rect(0, 0, 1024, 768));
  Shell::Get()->session_controller()->NotifyChromeTerminating();
  SendUpdates(window.get(), kMinRect, kMinFps + 5, 2 * kMinDuration);
  EXPECT_TRUE(observer_->IsEmpty());
}

TEST_F(VideoDetectorTest, ReportFullscreen) {
  UpdateDisplay("1024x768,1024x768");

  std::unique_ptr<aura::Window> window =
      CreateTestWindow(gfx::Rect(0, 0, 1024, 768));
  wm::WindowState* window_state = wm::GetWindowState(window.get());
  const wm::WMEvent toggle_fullscreen_event(wm::WM_EVENT_TOGGLE_FULLSCREEN);
  window_state->OnWMEvent(&toggle_fullscreen_event);
  ASSERT_TRUE(window_state->IsFullscreen());
  window->Focus();
  SendUpdates(window.get(), kMinRect, 2 * kMinFps, 2 * kMinDuration);
  EXPECT_EQ(VideoDetector::State::PLAYING_FULLSCREEN, observer_->PopState());
  EXPECT_TRUE(observer_->IsEmpty());

  // Make the window non-fullscreen.
  observer_->Reset();
  window_state->OnWMEvent(&toggle_fullscreen_event);
  ASSERT_FALSE(window_state->IsFullscreen());
  EXPECT_EQ(VideoDetector::State::PLAYING_WINDOWED, observer_->PopState());
  EXPECT_TRUE(observer_->IsEmpty());

  // Open a second, fullscreen window. Fullscreen video should still be reported
  // due to the second window being fullscreen. This avoids situations where
  // non-fullscreen video could be reported when multiple videos are playing in
  // fullscreen and non-fullscreen windows.
  observer_->Reset();
  std::unique_ptr<aura::Window> other_window =
      CreateTestWindow(gfx::Rect(1024, 0, 1024, 768));
  wm::WindowState* other_window_state = wm::GetWindowState(other_window.get());
  other_window_state->OnWMEvent(&toggle_fullscreen_event);
  ASSERT_TRUE(other_window_state->IsFullscreen());
  EXPECT_EQ(VideoDetector::State::PLAYING_FULLSCREEN, observer_->PopState());
  EXPECT_TRUE(observer_->IsEmpty());

  // Make the second window non-fullscreen and check that the observer is
  // immediately notified about windowed video.
  observer_->Reset();
  other_window_state->OnWMEvent(&toggle_fullscreen_event);
  ASSERT_FALSE(other_window_state->IsFullscreen());
  EXPECT_EQ(VideoDetector::State::PLAYING_WINDOWED, observer_->PopState());
  EXPECT_TRUE(observer_->IsEmpty());
}
*/

}  // namespace viz
