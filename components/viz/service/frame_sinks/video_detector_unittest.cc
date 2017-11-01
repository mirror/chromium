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
#include "components/viz/test/compositor_frame_helpers.h"
#include "components/viz/test/fake_compositor_frame_sink_client.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/viz/public/interfaces/compositing/video_detector_observer.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/gfx/geometry/rect.h"

namespace viz {

// Implementation that just records video state changes.
class TestObserver : public mojom::VideoDetectorObserver {
 public:
  TestObserver() : binding_(this) {}

  void Bind(mojom::VideoDetectorObserverRequest request) {
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
  bool PopState() {
    binding_.FlushForTesting();
    CHECK(!states_.empty());
    uint8_t first_state = states_.front();
    states_.pop_front();
    return first_state;
  }

  // mojom::VideoDetectorObserver implementation.
  void OnVideoActivityStarted() override { states_.push_back(true); }
  void OnVideoActivityEnded() override { states_.push_back(false); }

 private:
  // States in the order they were received.
  base::circular_deque<bool> states_;

  mojo::Binding<mojom::VideoDetectorObserver> binding_;

  DISALLOW_COPY_AND_ASSIGN(TestObserver);
};

class VideoDetectorTest : public testing::Test {
 public:
  VideoDetectorTest()
      : surface_aggregator_(frame_sink_manager_.surface_manager(),
                            nullptr,
                            false) {}
  ~VideoDetectorTest() override {}

  void SetUp() override {
    mojom::VideoDetectorObserverPtr video_detector_observer;
    observer_.Bind(mojo::MakeRequest(&video_detector_observer));
    frame_sink_manager_.AddVideoDetectorObserver(
        std::move(video_detector_observer));

    detector_ = frame_sink_manager_.video_detector_for_testing();

    now_ = base::TimeTicks::Now();
    detector_->set_now_for_testing(now_);

    root_frame_sink_ = CreateFrameSink();
    root_frame_sink_->SubmitCompositorFrame(
        local_surface_id_allocator_.GenerateId(), test::MakeCompositorFrame());
  }

  void TearDown() override {}

 protected:
  // Move |detector_|'s idea of the current time forward by |delta|.
  void AdvanceTime(base::TimeDelta delta) {
    now_ += delta;
    detector_->set_now_for_testing(now_);
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

  // Report updates to |client| of area |region| at a rate of
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
    detector_->set_now_for_testing(now_);
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

  bool TriggerInactivityTimeout() {
    return detector_->TriggerTimeoutForTesting();
  }

  // Constants placed here for convenience.
  static constexpr int kMinFps = VideoDetector::kMinFramesPerSecond;
  static constexpr gfx::Rect kMinRect =
      gfx::Rect(VideoDetector::kMinUpdateWidth,
                VideoDetector::kMinUpdateHeight);
  static constexpr base::TimeDelta kMinDuration =
      VideoDetector::kMinVideoDuration;
  static constexpr base::TimeDelta kTimeout = VideoDetector::kVideoTimeout;

  VideoDetector* detector_;  // not owned
  TestObserver observer_;

  // The current (fake) time used by |detector_|.
  base::TimeTicks now_;

 private:
  CompositorFrame MakeDamagedCompositorFrame(const gfx::Rect& damage) {
    constexpr gfx::Size kFrameSinkSize = gfx::Size(10000, 10000);
    CompositorFrame frame = test::MakeCompositorFrame(kFrameSinkSize);
    frame.render_pass_list.back()->damage_rect = damage;
    return frame;
  }

  FrameSinkManagerImpl frame_sink_manager_;
  FakeCompositorFrameSinkClient frame_sink_client_;
  LocalSurfaceIdAllocator local_surface_id_allocator_;
  SurfaceAggregator surface_aggregator_;
  std::unique_ptr<CompositorFrameSinkSupport> root_frame_sink_;
  std::set<CompositorFrameSinkSupport*> embedded_clients_;

  DISALLOW_COPY_AND_ASSIGN(VideoDetectorTest);
};

constexpr gfx::Rect VideoDetectorTest::kMinRect;
constexpr base::TimeDelta VideoDetectorTest::kMinDuration;
constexpr base::TimeDelta VideoDetectorTest::kTimeout;

// Verify that VideoDetector does not report clients with small damage rects.
TEST_F(VideoDetectorTest, DontReportWhenDamageTooSmall) {
  std::unique_ptr<CompositorFrameSinkSupport> frame_sink = CreateFrameSink();
  EmbedClient(frame_sink.get());
  gfx::Rect rect = kMinRect;
  rect.Inset(0, 0, 1, 0);
  SendUpdates(frame_sink.get(), rect, 2 * kMinFps, 2 * kMinDuration);
  EXPECT_TRUE(observer_.IsEmpty());
}

// Verify that VideoDetector does not report clients with a low frame rate.
TEST_F(VideoDetectorTest, DontReportWhenFramerateTooLow) {
  std::unique_ptr<CompositorFrameSinkSupport> frame_sink = CreateFrameSink();
  EmbedClient(frame_sink.get());
  SendUpdates(frame_sink.get(), kMinRect, kMinFps - 5, 2 * kMinDuration);
  EXPECT_TRUE(observer_.IsEmpty());
}

// Verify that VideoDetector does not report clients until they have played for
// the minimum necessary duration.
TEST_F(VideoDetectorTest, DontReportWhenNotPlayingLongEnough) {
  std::unique_ptr<CompositorFrameSinkSupport> frame_sink = CreateFrameSink();
  EmbedClient(frame_sink.get());
  SendUpdates(frame_sink.get(), kMinRect, 2 * kMinFps, 0.5 * kMinDuration);
  EXPECT_TRUE(observer_.IsEmpty());

  SendUpdates(frame_sink.get(), kMinRect, 2 * kMinFps, 0.6 * kMinDuration);
  EXPECT_TRUE(observer_.PopState());
  EXPECT_TRUE(observer_.IsEmpty());
}

// Verify that VideoDetector does not report clients that are not visible
// on screen.
TEST_F(VideoDetectorTest, DontReportWhenClientHidden) {
  std::unique_ptr<CompositorFrameSinkSupport> frame_sink = CreateFrameSink();

  SendUpdates(frame_sink.get(), kMinRect, kMinFps + 5, 2 * kMinDuration);
  EXPECT_TRUE(observer_.IsEmpty());

  // Make the client visible.
  observer_.Reset();
  AdvanceTime(kTimeout);
  EmbedClient(frame_sink.get());
  SendUpdates(frame_sink.get(), kMinRect, kMinFps + 5, 2 * kMinDuration);
  EXPECT_TRUE(observer_.PopState());
  EXPECT_TRUE(observer_.IsEmpty());
}

// Turn video activity on and off. Make sure the observers are notified
// properly.
TEST_F(VideoDetectorTest, ReportStartAndStop) {
  const base::TimeDelta kDuration =
      kMinDuration + base::TimeDelta::FromMilliseconds(100);
  std::unique_ptr<CompositorFrameSinkSupport> frame_sink = CreateFrameSink();
  EmbedClient(frame_sink.get());
  SendUpdates(frame_sink.get(), kMinRect, kMinFps + 5, kDuration);
  EXPECT_TRUE(observer_.PopState());
  EXPECT_TRUE(observer_.IsEmpty());
  AdvanceTime(kTimeout);
  EXPECT_TRUE(TriggerInactivityTimeout());
  EXPECT_FALSE(observer_.PopState());
  EXPECT_TRUE(observer_.IsEmpty());

  // The timer shouldn't be running anymore.
  EXPECT_FALSE(TriggerInactivityTimeout());

  // Start playing again.
  SendUpdates(frame_sink.get(), kMinRect, kMinFps + 5, kDuration);
  EXPECT_TRUE(observer_.PopState());
  EXPECT_TRUE(observer_.IsEmpty());

  AdvanceTime(kTimeout);
  EXPECT_TRUE(TriggerInactivityTimeout());
  EXPECT_FALSE(observer_.PopState());
  EXPECT_TRUE(observer_.IsEmpty());
}

// If there are multiple clients playing video, make sure that observers only
// receive a single notification.
TEST_F(VideoDetectorTest, ReportOnceForMultipleClients) {
  gfx::Rect kClientBounds(gfx::Point(), gfx::Size(1024, 768));
  std::unique_ptr<CompositorFrameSinkSupport> frame_sink1 = CreateFrameSink();
  std::unique_ptr<CompositorFrameSinkSupport> frame_sink2 = CreateFrameSink();
  EmbedClient(frame_sink1.get());
  EmbedClient(frame_sink2.get());

  // Even if there's video playing in both clients, the observer should only
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
  EXPECT_TRUE(observer_.PopState());
  EXPECT_TRUE(observer_.IsEmpty());
}

}  // namespace viz
