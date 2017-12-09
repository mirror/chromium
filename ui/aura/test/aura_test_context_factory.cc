// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/aura/test/aura_test_context_factory.h"

#include "cc/test/fake_output_surface.h"
#include "cc/test/test_context_provider.h"
#include "components/viz/test/test_layer_tree_frame_sink.h"
#include "ui/compositor/reflector.h"

namespace aura {
namespace test {
namespace {

class FrameSinkClient : public viz::TestLayerTreeFrameSinkClient {
 public:
  explicit FrameSinkClient(
      scoped_refptr<viz::ContextProvider> display_context_provider)
      : display_context_provider_(std::move(display_context_provider)) {
    NOTIMPLEMENTED();
  }

  // viz::TestLayerTreeFrameSinkClient:
  std::unique_ptr<viz::OutputSurface> CreateDisplayOutputSurface(
      scoped_refptr<viz::ContextProvider> compositor_context_provider)
      override {
    return cc::FakeOutputSurface::Create3d(
        std::move(display_context_provider_));
  }
  void DisplayReceivedLocalSurfaceId(
      const viz::LocalSurfaceId& local_surface_id) override {
    NOTIMPLEMENTED();
  }
  void DisplayReceivedCompositorFrame(
      const viz::CompositorFrame& frame) override {
    NOTIMPLEMENTED();
  }
  void DisplayWillDrawAndSwap(
      bool will_draw_and_swap,
      const viz::RenderPassList& render_passes) override {
    NOTIMPLEMENTED();
  }
  void DisplayDidDrawAndSwap() override { NOTIMPLEMENTED(); }

 private:
  scoped_refptr<viz::ContextProvider> display_context_provider_;

  DISALLOW_COPY_AND_ASSIGN(FrameSinkClient);
};

}  // namespace

AuraTestContextFactory::AuraTestContextFactory()
    : frame_sink_id_allocator_(99u /* client_id */) {
  frame_sink_manager_impl_.SetLocalClient(&host_frame_sink_manager_);
  host_frame_sink_manager_.SetLocalManager(&frame_sink_manager_impl_);
}

AuraTestContextFactory::~AuraTestContextFactory() = default;

void AuraTestContextFactory::CreateLayerTreeFrameSink(
    base::WeakPtr<ui::Compositor> compositor) {
  scoped_refptr<cc::TestContextProvider> context_provider =
      cc::TestContextProvider::Create();
  std::unique_ptr<FrameSinkClient> frame_sink_client =
      std::make_unique<FrameSinkClient>(context_provider);
  constexpr bool synchronous_composite = false;
  constexpr bool disable_display_vsync = false;
  const double refresh_rate = GetRefreshRate();
  auto frame_sink = std::make_unique<viz::TestLayerTreeFrameSink>(
      context_provider, cc::TestContextProvider::CreateWorker(), nullptr,
      GetGpuMemoryBufferManager(), renderer_settings(),
      base::ThreadTaskRunnerHandle::Get().get(), synchronous_composite,
      disable_display_vsync, refresh_rate);
  frame_sink->SetClient(frame_sink_client.get());
  compositor->SetLayerTreeFrameSink(std::move(frame_sink));
  frame_sink_clients_.insert(std::move(frame_sink_client));
}

std::unique_ptr<ui::Reflector> AuraTestContextFactory::CreateReflector(
    ui::Compositor* mirrored_compositor,
    ui::Layer* mirroring_layer) {
  NOTIMPLEMENTED();
  return nullptr;
}

void AuraTestContextFactory::RemoveReflector(ui::Reflector* reflector) {
  NOTIMPLEMENTED();
}

viz::FrameSinkId AuraTestContextFactory::AllocateFrameSinkId() {
  return frame_sink_id_allocator_.NextFrameSinkId();
}

viz::FrameSinkManagerImpl* AuraTestContextFactory::GetFrameSinkManager() {
  return &frame_sink_manager_impl_;
}

viz::HostFrameSinkManager* AuraTestContextFactory::GetHostFrameSinkManager() {
  return &host_frame_sink_manager_;
}

void AuraTestContextFactory::SetDisplayVisible(ui::Compositor* compositor,
                                               bool visible) {
  NOTIMPLEMENTED();
}

void AuraTestContextFactory::ResizeDisplay(ui::Compositor* compositor,
                                           const gfx::Size& size) {
  NOTIMPLEMENTED();
}

void AuraTestContextFactory::SetDisplayColorMatrix(ui::Compositor* compositor,
                                                   const SkMatrix44& matrix) {
  NOTIMPLEMENTED();
}

void AuraTestContextFactory::SetDisplayColorSpace(
    ui::Compositor* compositor,
    const gfx::ColorSpace& blending_color_space,
    const gfx::ColorSpace& output_color_space) {
  NOTIMPLEMENTED();
}

void AuraTestContextFactory::SetAuthoritativeVSyncInterval(
    ui::Compositor* compositor,
    base::TimeDelta interval) {
  NOTIMPLEMENTED();
}

void AuraTestContextFactory::SetDisplayVSyncParameters(
    ui::Compositor* compositor,
    base::TimeTicks timebase,
    base::TimeDelta interval) {
  NOTIMPLEMENTED();
}

void AuraTestContextFactory::IssueExternalBeginFrame(
    ui::Compositor* compositor,
    const viz::BeginFrameArgs& args) {
  NOTIMPLEMENTED();
}

void AuraTestContextFactory::SetOutputIsSecure(ui::Compositor* compositor,
                                               bool secure) {
  NOTIMPLEMENTED();
}

}  // namespace test
}  // namespace aura
