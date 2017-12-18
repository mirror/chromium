// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/compositor/test/test_process_transport_factory.h"

#include <limits>

#include "cc/test/fake_layer_tree_frame_sink.h"
#include "ui/compositor/reflector.h"

namespace content {
namespace {

// TODO(kylechar): Pick a client id for the browser process to use.
constexpr uint32_t kDefaultClientId = std::numeric_limits<uint32_t>::max();

}  // namespace

TestProcessTransportFactory::TestProcessTransportFactory()
    : frame_sink_id_allocator_(kDefaultClientId) {
  viz::mojom::FrameSinkManagerPtr frame_sink_manager;
  viz::mojom::FrameSinkManagerRequest frame_sink_manager_request =
      mojo::MakeRequest(&frame_sink_manager);
  viz::mojom::FrameSinkManagerClientPtr frame_sink_manager_client;
  viz::mojom::FrameSinkManagerClientRequest frame_sink_manager_client_request =
      mojo::MakeRequest(&frame_sink_manager_client);

  // Bind endpoints in HostFrameSinkManager.
  host_frame_sink_manager_.BindAndSetManager(
      std::move(frame_sink_manager_client_request), nullptr,
      std::move(frame_sink_manager));

  // Bind endpoints in TestFrameSinkManagerImpl. For non-tests there would be a
  // FrameSinkManagerImpl running in another process and these interface
  // endpoints would be bound there.
  frame_sink_manager_impl_.BindRequest(std::move(frame_sink_manager_request),
                                       std::move(frame_sink_manager_client));
}

TestProcessTransportFactory::~TestProcessTransportFactory() {
  for (auto& observer : observer_list_)
    observer.OnLostResources();
}

void TestProcessTransportFactory::CreateLayerTreeFrameSink(
    base::WeakPtr<ui::Compositor> compositor) {
  auto frame_sink = cc::FakeLayerTreeFrameSink::Create3d();
  compositor->SetLayerTreeFrameSink(std::move(frame_sink));
}

scoped_refptr<viz::ContextProvider>
TestProcessTransportFactory::SharedMainThreadContextProvider() {
  NOTREACHED();
  return nullptr;
}

void TestProcessTransportFactory::RemoveCompositor(ui::Compositor* compositor) {
}

double TestProcessTransportFactory::GetRefreshRate() const {
  return 200.0;
}

gpu::GpuMemoryBufferManager*
TestProcessTransportFactory::GetGpuMemoryBufferManager() {
  return &gpu_memory_buffer_manager_;
}

cc::TaskGraphRunner* TestProcessTransportFactory::GetTaskGraphRunner() {
  return &task_graph_runner_;
}

const viz::ResourceSettings& TestProcessTransportFactory::GetResourceSettings()
    const {
  return renderer_settings_.resource_settings;
}

void TestProcessTransportFactory::AddObserver(
    ui::ContextFactoryObserver* observer) {
  observer_list_.AddObserver(observer);
}

void TestProcessTransportFactory::RemoveObserver(
    ui::ContextFactoryObserver* observer) {
  observer_list_.RemoveObserver(observer);
}

std::unique_ptr<ui::Reflector> TestProcessTransportFactory::CreateReflector(
    ui::Compositor* source,
    ui::Layer* target) {
  // TODO(crbug.com/601869): Reflector needs to be rewritten for viz.
  NOTIMPLEMENTED();
  return nullptr;
}

void TestProcessTransportFactory::RemoveReflector(ui::Reflector* reflector) {
  // TODO(crbug.com/601869): Reflector needs to be rewritten for viz.
  NOTIMPLEMENTED();
}

viz::FrameSinkId TestProcessTransportFactory::AllocateFrameSinkId() {
  return frame_sink_id_allocator_.NextFrameSinkId();
}

viz::HostFrameSinkManager*
TestProcessTransportFactory::GetHostFrameSinkManager() {
  return &host_frame_sink_manager_;
}

void TestProcessTransportFactory::SetDisplayVisible(ui::Compositor* compositor,
                                                    bool visible) {}

void TestProcessTransportFactory::ResizeDisplay(ui::Compositor* compositor,
                                                const gfx::Size& size) {}

void TestProcessTransportFactory::SetDisplayColorMatrix(
    ui::Compositor* compositor,
    const SkMatrix44& matrix) {}

void TestProcessTransportFactory::SetDisplayColorSpace(
    ui::Compositor* compositor,
    const gfx::ColorSpace& blending_color_space,
    const gfx::ColorSpace& output_color_space) {}

void TestProcessTransportFactory::SetAuthoritativeVSyncInterval(
    ui::Compositor* compositor,
    base::TimeDelta interval) {}

void TestProcessTransportFactory::SetDisplayVSyncParameters(
    ui::Compositor* compositor,
    base::TimeTicks timebase,
    base::TimeDelta interval) {}

void TestProcessTransportFactory::IssueExternalBeginFrame(
    ui::Compositor* compositor,
    const viz::BeginFrameArgs& args) {}

void TestProcessTransportFactory::SetOutputIsSecure(ui::Compositor* compositor,
                                                    bool secure) {}

viz::FrameSinkManagerImpl* TestProcessTransportFactory::GetFrameSinkManager() {
  NOTREACHED();  // This should never be called when running with --enable-viz.
  return nullptr;
}

bool TestProcessTransportFactory::IsGpuCompositingDisabled() {
  return false;
}

ui::ContextFactory* TestProcessTransportFactory::GetContextFactory() {
  return this;
}

ui::ContextFactoryPrivate*
TestProcessTransportFactory::GetContextFactoryPrivate() {
  return this;
}

viz::GLHelper* TestProcessTransportFactory::GetGLHelper() {
  NOTREACHED();  // This should never be called when running with --enable-viz.
  return nullptr;
}

#if defined(OS_MACOSX)
void TestProcessTransportFactory::SetCompositorSuspendedForRecycle(
    ui::Compositor* compositor,
    bool suspended) {
  NOTIMPLEMENTED();
}
#endif

}  // namespace content
