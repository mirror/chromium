// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/compositor/test/test_image_transport_factory.h"

#include <limits>

#include "base/command_line.h"
#include "base/macros.h"
#include "base/observer_list.h"
#include "build/build_config.h"
#include "cc/test/fake_layer_tree_frame_sink.h"
#include "cc/test/test_task_graph_runner.h"
#include "components/viz/common/display/renderer_settings.h"
#include "components/viz/common/surfaces/frame_sink_id_allocator.h"
#include "components/viz/common/switches.h"
#include "components/viz/host/host_frame_sink_manager.h"
#include "components/viz/test/test_frame_sink_manager.h"
#include "components/viz/test/test_gpu_memory_buffer_manager.h"
#include "content/browser/compositor/image_transport_factory.h"
#include "content/browser/compositor/test/no_transport_image_transport_factory.h"
#include "ui/compositor/compositor.h"
#include "ui/compositor/reflector.h"

namespace content {
namespace {

// TODO(kylechar): Use the same client id for the browser everywhere.
constexpr uint32_t kDefaultClientId = std::numeric_limits<uint32_t>::max();

// This class replaces VizProcessTransportFactory for unittests. A fake
// LayerTreeFrameSink implementation is used.
class TestProcessTransportFactory : public ui::ContextFactory,
                                    public ui::ContextFactoryPrivate,
                                    public ImageTransportFactory {
 public:
  TestProcessTransportFactory();
  ~TestProcessTransportFactory() override;

  // ui::ContextFactory implementation.
  void CreateLayerTreeFrameSink(
      base::WeakPtr<ui::Compositor> compositor) override;
  scoped_refptr<viz::ContextProvider> SharedMainThreadContextProvider()
      override;
  void RemoveCompositor(ui::Compositor* compositor) override;
  double GetRefreshRate() const override;
  gpu::GpuMemoryBufferManager* GetGpuMemoryBufferManager() override;
  cc::TaskGraphRunner* GetTaskGraphRunner() override;
  const viz::ResourceSettings& GetResourceSettings() const override;
  void AddObserver(ui::ContextFactoryObserver* observer) override;
  void RemoveObserver(ui::ContextFactoryObserver* observer) override;

  // ui::ContextFactoryPrivate implementation.
  std::unique_ptr<ui::Reflector> CreateReflector(ui::Compositor* source,
                                                 ui::Layer* target) override;
  void RemoveReflector(ui::Reflector* reflector) override;
  viz::FrameSinkId AllocateFrameSinkId() override;
  viz::HostFrameSinkManager* GetHostFrameSinkManager() override;
  void SetDisplayVisible(ui::Compositor* compositor, bool visible) override {}
  void ResizeDisplay(ui::Compositor* compositor,
                     const gfx::Size& size) override {}
  void SetDisplayColorMatrix(ui::Compositor* compositor,
                             const SkMatrix44& matrix) override {}
  void SetDisplayColorSpace(
      ui::Compositor* compositor,
      const gfx::ColorSpace& blending_color_space,
      const gfx::ColorSpace& output_color_space) override {}
  void SetAuthoritativeVSyncInterval(ui::Compositor* compositor,
                                     base::TimeDelta interval) override {}
  void SetDisplayVSyncParameters(ui::Compositor* compositor,
                                 base::TimeTicks timebase,
                                 base::TimeDelta interval) override {}
  void IssueExternalBeginFrame(ui::Compositor* compositor,
                               const viz::BeginFrameArgs& args) override {}
  void SetOutputIsSecure(ui::Compositor* compositor, bool secure) override {}
  viz::FrameSinkManagerImpl* GetFrameSinkManager() override;

  // ImageTransportFactory implementation.
  bool IsGpuCompositingDisabled() override;
  ui::ContextFactory* GetContextFactory() override;
  ui::ContextFactoryPrivate* GetContextFactoryPrivate() override;
  viz::GLHelper* GetGLHelper() override;
#if defined(OS_MACOSX)
  void SetCompositorSuspendedForRecycle(ui::Compositor* compositor,
                                        bool suspended) override;
#endif

 private:
  cc::TestTaskGraphRunner task_graph_runner_;
  viz::TestGpuMemoryBufferManager gpu_memory_buffer_manager_;
  viz::RendererSettings renderer_settings_;
  viz::FrameSinkIdAllocator frame_sink_id_allocator_;
  base::ObserverList<ui::ContextFactoryObserver> observer_list_;
  viz::HostFrameSinkManager host_frame_sink_manager_;
  viz::TestFrameSinkManagerImpl frame_sink_manager_impl_;

  DISALLOW_COPY_AND_ASSIGN(TestProcessTransportFactory);
};

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
  // We might want to call HostFrameSinkManager::CreateRootCompositorFrameSink()
  // here at some point.
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

}  // namespace

std::unique_ptr<ImageTransportFactory> CreateImageTransportFactoryForTesting() {
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(switches::kEnableViz))
    return std::make_unique<TestProcessTransportFactory>();

  return std::make_unique<NoTransportImageTransportFactory>();
}

}  // namespace content
