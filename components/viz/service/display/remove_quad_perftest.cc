// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <vector>

#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "base/time/time.h"
#include "cc/base/lap_timer.h"
#include "components/viz/common/display/renderer_settings.h"
#include "components/viz/common/quads/compositor_frame.h"
#include "components/viz/common/quads/draw_quad.h"
#include "components/viz/common/quads/render_pass.h"
#include "components/viz/common/quads/texture_draw_quad.h"
#include "components/viz/common/resources/shared_bitmap_manager.h"
#include "components/viz/common/surfaces/frame_sink_id.h"
#include "components/viz/service/display/display.h"
#include "components/viz/service/display/display_scheduler.h"
#include "components/viz/service/display/output_surface.h"
#include "components/viz/service/display/texture_mailbox_deleter.h"
#include "components/viz/service/display_embedder/server_shared_bitmap_manager.h"
#include "components/viz/test/compositor_frame_helpers.h"
#include "components/viz/test/test_gpu_memory_buffer_manager.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/perf/perf_test.h"
#include "third_party/skia/include/core/SkBlendMode.h"

namespace viz {
namespace {

static const int kTimeLimitMillis = 2000;
static const int kWarmupRuns = 5;
static const int kTimeCheckInterval = 10;
static const int kHeight = 1000;
static const int kWidth = 1000;

SharedQuadState* CreateSharedQuadState(RenderPass* render_pass,
                                       gfx::Rect rect) {
  gfx::Transform quad_transform = gfx::Transform();
  bool is_clipped = false;
  bool are_contents_opaque = true;
  float opacity = 1.f;
  int sorting_context_id = 65536;
  SkBlendMode blend_mode = SkBlendMode::kSrcOver;

  SharedQuadState* state = render_pass->CreateAndAppendSharedQuadState();
  state->SetAll(quad_transform, rect, rect, rect, is_clipped,
                are_contents_opaque, opacity, blend_mode, sorting_context_id);
  return state;
}

class RemoveQuadPerfTest : public testing::Test {
 public:
  RemoveQuadPerfTest()
      : timer_(kWarmupRuns,
               base::TimeDelta::FromMilliseconds(kTimeLimitMillis),
               kTimeCheckInterval) {}

  void CreateRenderPass(int shared_quad_state_count, int quad_count) {
    int shared_quad_state_height = kHeight / shared_quad_state_count;
    int shared_quad_state_width = kWidth / shared_quad_state_count;
    int quad_height = shared_quad_state_height / quad_count;
    int quad_width = shared_quad_state_width / quad_count;
    int i = 0;
    int j = 0;
    while (i + shared_quad_state_height <= kWidth) {
      while (j + shared_quad_state_height <= kHeight) {
        gfx::Rect rect(i, j, shared_quad_state_height, shared_quad_state_width);
        SharedQuadState* new_shared_state(CreateSharedQuadState(
            frame_->render_pass_list.front().get(), rect));
        AppendQuads(new_shared_state, quad_height, quad_width);
        j += shared_quad_state_height;
      }
      j = 0;
      i += shared_quad_state_width;
    }
  }

  void CleanUpRenderPass() { frame_.reset(); }

  void AppendQuads(SharedQuadState* shared_quad_state,
                   int quad_height,
                   int quad_width) {
    int x_start = shared_quad_state->visible_quad_layer_rect.x();
    int x_end = x_start + shared_quad_state->visible_quad_layer_rect.width();
    int y_start = shared_quad_state->visible_quad_layer_rect.y();
    int y_end = y_start + shared_quad_state->visible_quad_layer_rect.height();
    int i = x_start;
    int j = y_start;
    while (i + quad_width <= x_end) {
      while (j + quad_height <= y_end) {
        auto* quad = frame_->render_pass_list.front()
                         ->CreateAndAppendDrawQuad<TextureDrawQuad>();
        gfx::Rect rect(i, j, quad_width, quad_height);
        bool needs_blending = false;
        ResourceId resource_id = 1;
        bool premultiplied_alpha = true;
        gfx::PointF uv_top_left(0, 0);
        gfx::PointF uv_bottom_right(1, 1);
        SkColor background_color = SK_ColorRED;
        float vertex_opacity[4] = {1.f, 1.f, 1.f, 1.f};
        bool y_flipped = false;
        bool nearest_neighbor = true;

        quad->SetNew(shared_quad_state, rect, rect, needs_blending, resource_id,
                     premultiplied_alpha, uv_top_left, uv_bottom_right,
                     background_color, vertex_opacity, y_flipped,
                     nearest_neighbor, false);
        j += quad_height;
      }
      j = y_start;
      i += quad_width;
    }
  }

  void RunIterateResourceTest(const std::string& test_name,
                              int shared_quad_state_count,
                              int quad_count) {
    frame_ = base::MakeUnique<CompositorFrame>(test::MakeCompositorFrame());
    CreateRenderPass(shared_quad_state_count, quad_count);
    Display display(new ServerSharedBitmapManager(),
                    new TestGpuMemoryBufferManager(), RendererSettings(),
                    FrameSinkId(), nullptr, nullptr, nullptr);

    timer_.Reset();
    do {
      display.RemoveOverdrawQuads(frame_.get());
      timer_.NextLap();
    } while (!timer_.HasTimeLimitExpired());

    perf_test::PrintResult("RemoveOverdrawDraws Iterates quads: ", "",
                           test_name, timer_.LapsPerSecond(), "runs/s", true);
    CleanUpRenderPass();
  }

 private:
  std::unique_ptr<CompositorFrame> frame_;
  cc::LapTimer timer_;
};

TEST_F(RemoveQuadPerfTest, IterateResources) {
  RunIterateResourceTest("4_sqs_4_quads", 2, 2);
  RunIterateResourceTest("4_sqs_100_quads", 2, 10);
  RunIterateResourceTest("100_sqs_4_quads", 10, 2);
  RunIterateResourceTest("100_sqs_100_quads", 10, 10);
}

}  // namespace
}  // namespace viz
