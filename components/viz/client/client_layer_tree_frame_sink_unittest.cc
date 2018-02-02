// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/client/client_layer_tree_frame_sink.h"

#include <memory>

#include "base/bind.h"
#include "base/memory/scoped_refptr.h"
#include "base/run_loop.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread.h"
#include "cc/test/fake_layer_tree_frame_sink_client.h"
#include "cc/test/test_context_provider.h"
#include "components/viz/client/local_surface_id_provider.h"
#include "components/viz/common/quads/surface_draw_quad.h"
#include "components/viz/test/compositor_frame_helpers.h"
#include "components/viz/test/test_gpu_memory_buffer_manager.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "services/viz/public/interfaces/compositing/compositor_frame_sink.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace viz {
namespace {

// Used to track the thread DidLoseLayerTreeFrameSink() is called on (and quit
// a RunLoop).
class ThreadTrackingLayerTreeFrameSinkClient
    : public cc::FakeLayerTreeFrameSinkClient {
 public:
  ThreadTrackingLayerTreeFrameSinkClient(
      base::PlatformThreadId* called_thread_id,
      base::RunLoop* run_loop)
      : called_thread_id_(called_thread_id), run_loop_(run_loop) {}
  ~ThreadTrackingLayerTreeFrameSinkClient() override = default;

  // cc::FakeLayerTreeFrameSinkClient:
  void DidLoseLayerTreeFrameSink() override {
    EXPECT_FALSE(did_lose_layer_tree_frame_sink_called());
    cc::FakeLayerTreeFrameSinkClient::DidLoseLayerTreeFrameSink();
    *called_thread_id_ = base::PlatformThread::CurrentId();
    run_loop_->Quit();
  }

 private:
  base::PlatformThreadId* called_thread_id_;
  base::RunLoop* run_loop_;

  DISALLOW_COPY_AND_ASSIGN(ThreadTrackingLayerTreeFrameSinkClient);
};

TEST(ClientLayerTreeFrameSinkTest,
     DidLoseLayerTreeFrameSinkCalledOnConnectionError) {
  base::Thread bg_thread("BG Thread");
  bg_thread.Start();

  scoped_refptr<cc::TestContextProvider> provider =
      cc::TestContextProvider::Create();
  TestGpuMemoryBufferManager test_gpu_memory_buffer_manager;

  mojom::CompositorFrameSinkPtrInfo sink_info;
  mojom::CompositorFrameSinkRequest sink_request =
      mojo::MakeRequest(&sink_info);
  mojom::CompositorFrameSinkClientPtr client;
  mojom::CompositorFrameSinkClientRequest client_request =
      mojo::MakeRequest(&client);

  ClientLayerTreeFrameSink::InitParams init_params;
  init_params.compositor_task_runner = bg_thread.task_runner();
  init_params.gpu_memory_buffer_manager = &test_gpu_memory_buffer_manager;
  init_params.pipes.compositor_frame_sink_info = std::move(sink_info);
  init_params.pipes.client_request = std::move(client_request);
  init_params.local_surface_id_provider =
      std::make_unique<DefaultLocalSurfaceIdProvider>();
  init_params.enable_surface_synchronization = true;
  ClientLayerTreeFrameSink layer_tree_frame_sink(std::move(provider), nullptr,
                                                 &init_params);

  base::PlatformThreadId called_thread_id = base::kInvalidThreadId;
  base::RunLoop close_run_loop;
  ThreadTrackingLayerTreeFrameSinkClient frame_sink_client(&called_thread_id,
                                                           &close_run_loop);

  auto bind_in_background =
      [](ClientLayerTreeFrameSink* layer_tree_frame_sink,
         ThreadTrackingLayerTreeFrameSinkClient* frame_sink_client) {
        layer_tree_frame_sink->BindToClient(frame_sink_client);
      };
  bg_thread.task_runner()->PostTask(
      FROM_HERE, base::BindOnce(bind_in_background,
                                base::Unretained(&layer_tree_frame_sink),
                                base::Unretained(&frame_sink_client)));
  // Closes the pipe, which should trigger calling DidLoseLayerTreeFrameSink()
  // (and quitting the RunLoop). There is no need to wait for BindToClient()
  // to complete as mojo::Binding error callbacks are processed asynchronously.
  sink_request = mojom::CompositorFrameSinkRequest();
  close_run_loop.Run();

  EXPECT_NE(base::kInvalidThreadId, called_thread_id);
  EXPECT_EQ(called_thread_id, bg_thread.GetThreadId());

  // DetachFromClient() has to be called on the background thread.
  base::RunLoop detach_run_loop;
  auto detach_in_background =
      [](ClientLayerTreeFrameSink* layer_tree_frame_sink,
         base::RunLoop* detach_run_loop) {
        layer_tree_frame_sink->DetachFromClient();
        detach_run_loop->Quit();
      };
  bg_thread.task_runner()->PostTask(
      FROM_HERE, base::BindOnce(detach_in_background,
                                base::Unretained(&layer_tree_frame_sink),
                                base::Unretained(&detach_run_loop)));
  detach_run_loop.Run();
}

class TestCompositorFrameSinkImpl : public mojom::CompositorFrameSink {
 public:
  explicit TestCompositorFrameSinkImpl(
      mojom::CompositorFrameSinkRequest request)
      : compositor_frame_sink_binding_(this, std::move(request)) {}
  ~TestCompositorFrameSinkImpl() override = default;

  // mojom::CompositorFrameSink:
  void SetNeedsBeginFrame(bool needs_begin_frame) override {}
  void SetWantsAnimateOnlyBeginFrames() override {}
  void SubmitCompositorFrame(const LocalSurfaceId& local_surface_id,
                             CompositorFrame frame,
                             mojom::HitTestRegionListPtr hit_test_region_list,
                             uint64_t submit_time) override {
    hit_test_region_list_ = std::move(hit_test_region_list);
  }
  void DidNotProduceFrame(const BeginFrameAck& begin_frame_ack) override {}

  void Flush() { compositor_frame_sink_binding_.FlushForTesting(); }

  mojom::HitTestRegionListPtr hit_test_region_list_;

 private:
  mojo::Binding<mojom::CompositorFrameSink> compositor_frame_sink_binding_;

  DISALLOW_COPY_AND_ASSIGN(TestCompositorFrameSinkImpl);
};

CompositorFrame MakeCompositorFrameWithChildSurface(gfx::Rect rect) {
  auto pass = RenderPass::Create();
  pass->SetNew(1, rect, rect, gfx::Transform());

  auto* shared_state = pass->CreateAndAppendSharedQuadState();
  shared_state->SetAll(gfx::Transform(), rect, rect, rect, false, false, 1,
                       SkBlendMode::kSrcOver, 0);

  gfx::Size child_size(200, 100);

  LocalSurfaceId child_local_surface_id(2, base::UnguessableToken::Create());
  FrameSinkId frame_sink_id(2, 0);
  SurfaceId child_surface_id(frame_sink_id, child_local_surface_id);

  auto* surface_quad = pass->CreateAndAppendDrawQuad<SurfaceDrawQuad>();
  surface_quad->SetNew(pass->shared_quad_state_list.back(),
                       gfx::Rect(child_size), gfx::Rect(child_size),
                       child_surface_id, base::nullopt, SK_ColorWHITE, false);

  auto compositor_frame =
      CompositorFrameBuilder().AddRenderPass(std::move(pass)).Build();
  return compositor_frame;
}

constexpr gfx::Rect kFrameRect(0, 0, 1024, 768);

// Test to ensure that when hit test data is created from a CompositorFrame
// that the size matches the bounds and that the flags are set correctly.
TEST(ClientLayerTreeFrameSinkTest, HitTestDataFromCompositorFrame) {
  scoped_refptr<cc::TestContextProvider> provider =
      cc::TestContextProvider::Create();
  TestGpuMemoryBufferManager test_gpu_memory_buffer_manager;

  mojom::CompositorFrameSinkPtrInfo sink_info;
  mojom::CompositorFrameSinkRequest sink_request =
      mojo::MakeRequest(&sink_info);
  mojom::CompositorFrameSinkClientPtr client;
  mojom::CompositorFrameSinkClientRequest client_request =
      mojo::MakeRequest(&client);

  TestCompositorFrameSinkImpl test_compositor_frame_sink_impl(
      std::move(sink_request));

  ClientLayerTreeFrameSink::InitParams init_params;
  init_params.compositor_task_runner = base::ThreadTaskRunnerHandle::Get();

  init_params.gpu_memory_buffer_manager = &test_gpu_memory_buffer_manager;
  init_params.pipes.compositor_frame_sink_info = std::move(sink_info);
  init_params.pipes.client_request = std::move(client_request);
  init_params.local_surface_id_provider =
      std::make_unique<DefaultLocalSurfaceIdProvider>();
  init_params.enable_surface_synchronization = true;
  init_params.use_viz_hit_test = true;
  ClientLayerTreeFrameSink layer_tree_frame_sink(std::move(provider), nullptr,
                                                 &init_params);

  cc::FakeLayerTreeFrameSinkClient frame_sink_client;

  layer_tree_frame_sink.BindToClient(&frame_sink_client);
  LocalSurfaceId local_surface_id(1, base::UnguessableToken::Create());
  layer_tree_frame_sink.SetLocalSurfaceId(local_surface_id);

  // Ensure that a CompositorFrame without a child surface sets kHitTestMine.
  CompositorFrame compositor_frame =
      CompositorFrameBuilder().AddRenderPass(kFrameRect, kFrameRect).Build();

  layer_tree_frame_sink.SubmitCompositorFrame(std::move(compositor_frame));
  test_compositor_frame_sink_impl.Flush();
  EXPECT_EQ(mojom::kHitTestMouse | mojom::kHitTestTouch | mojom::kHitTestMine,
            test_compositor_frame_sink_impl.hit_test_region_list_->flags);
  EXPECT_EQ(kFrameRect,
            test_compositor_frame_sink_impl.hit_test_region_list_->bounds);

  // Ensure that a CompositorFrame with a child surface sets kHitTestAsk.
  compositor_frame = MakeCompositorFrameWithChildSurface(kFrameRect);
  layer_tree_frame_sink.SubmitCompositorFrame(std::move(compositor_frame));
  test_compositor_frame_sink_impl.Flush();
  EXPECT_EQ(mojom::kHitTestMouse | mojom::kHitTestTouch | mojom::kHitTestAsk,
            test_compositor_frame_sink_impl.hit_test_region_list_->flags);
  EXPECT_EQ(kFrameRect,
            test_compositor_frame_sink_impl.hit_test_region_list_->bounds);
}

}  // namespace
}  // namespace viz
