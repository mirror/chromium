// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/compositor/viz_process_transport_factory.h"

#include <utility>

#include "base/single_thread_task_runner.h"
#include "components/viz/client/client_layer_tree_frame_sink.h"
#include "components/viz/client/local_surface_id_provider.h"
#include "components/viz/common/gpu/context_provider.h"
#include "components/viz/host/host_frame_sink_manager.h"
#include "components/viz/host/renderer_settings_creation.h"
#include "content/browser/browser_main_loop.h"
#include "content/browser/browser_thread_impl.h"
#include "content/browser/gpu/compositor_util.h"
#include "content/browser/gpu/gpu_process_host.h"
#include "content/common/gpu_stream_constants.h"
#include "content/public/common/content_client.h"
#include "gpu/ipc/client/gpu_channel_host.h"
#include "services/ui/public/cpp/gpu/context_provider_command_buffer.h"
#include "ui/compositor/reflector.h"

namespace content {
namespace {

// The client_id used here should not conflict with the client_id generated
// from RenderWidgetHostImpl.
constexpr uint32_t kDefaultClientId = 0u;

scoped_refptr<viz::ContextProvider> CreateContextProvider(
    scoped_refptr<gpu::GpuChannelHost> gpu_channel_host) {
  constexpr bool kAutomaticFlushes = false;
  constexpr bool kSupportLocking = false;

  gpu::gles2::ContextCreationAttribHelper attributes;
  attributes.alpha_size = -1;
  attributes.depth_size = 0;
  attributes.stencil_size = 0;
  attributes.samples = 0;
  attributes.sample_buffers = 0;
  attributes.bind_generates_resource = false;
  attributes.lose_context_when_out_of_memory = true;
  attributes.buffer_preserved = false;

  GURL url("chrome://gpu/VizProcessTransportFactory::CreateContextProvider");
  return base::MakeRefCounted<ui::ContextProviderCommandBuffer>(
      std::move(gpu_channel_host), kGpuStreamIdDefault, kGpuStreamPriorityUI,
      gpu::kNullSurfaceHandle, url, kAutomaticFlushes, kSupportLocking,
      gpu::SharedMemoryLimits(), attributes,
      nullptr /* shared_context_provider */,
      ui::command_buffer_metrics::MUS_CLIENT_CONTEXT);
}

}  // namespace

VizProcessTransportFactory::VizProcessTransportFactory(
    gpu::GpuChannelEstablishFactory* gpu_channel_establish_factory,
    scoped_refptr<base::SingleThreadTaskRunner> resize_task_runner)
    : gpu_channel_establish_factory_(gpu_channel_establish_factory),
      resize_task_runner_(std::move(resize_task_runner)),
      frame_sink_id_allocator_(kDefaultClientId),
      renderer_settings_(
          viz::CreateRendererSettings(CreateBufferToTextureTargetMap())),
      weak_ptr_factory_(this) {
  DCHECK(gpu_channel_establish_factory_);
}

VizProcessTransportFactory::~VizProcessTransportFactory() = default;

void VizProcessTransportFactory::ConnectHostFrameSinkManager() {
  viz::mojom::FrameSinkManagerPtr frame_sink_manager;
  viz::mojom::FrameSinkManagerRequest frame_sink_manager_request =
      mojo::MakeRequest(&frame_sink_manager);
  viz::mojom::FrameSinkManagerClientPtr frame_sink_manager_client;
  viz::mojom::FrameSinkManagerClientRequest frame_sink_manager_client_request =
      mojo::MakeRequest(&frame_sink_manager_client);

  // Setup HostFrameSinkManager with interface endpoints.
  GetHostFrameSinkManager()->BindAndSetManager(
      std::move(frame_sink_manager_client_request), resize_task_runner_,
      std::move(frame_sink_manager));

  // Hop to the IO thread, then send the other side of interface to viz process.
  BrowserThread::PostTask(BrowserThread::IO, FROM_HERE,
                          base::BindOnce(
                              [](viz::mojom::FrameSinkManagerRequest request,
                                 viz::mojom::FrameSinkManagerClientPtr client) {
                                GpuProcessHost::Get()->ConnectFrameSinkManager(
                                    std::move(request), std::move(client));
                              },
                              std::move(frame_sink_manager_request),
                              std::move(frame_sink_manager_client)));
}

void VizProcessTransportFactory::CreateLayerTreeFrameSink(
    base::WeakPtr<ui::Compositor> compositor) {
  gpu_channel_establish_factory_->EstablishGpuChannel(base::Bind(
      &VizProcessTransportFactory::CreateLayerTreeFrameSinkForGpuChannel,
      weak_ptr_factory_.GetWeakPtr(), compositor));
}

scoped_refptr<viz::ContextProvider>
VizProcessTransportFactory::SharedMainThreadContextProvider() {
  NOTIMPLEMENTED();
  return nullptr;
}

void VizProcessTransportFactory::RemoveCompositor(ui::Compositor* compositor) {
  compositor_data_map_.erase(compositor);
}

double VizProcessTransportFactory::GetRefreshRate() const {
  // TODO(kylechar): Delete this function from ContextFactoryPrivate.
  return 60.0;
}

gpu::GpuMemoryBufferManager*
VizProcessTransportFactory::GetGpuMemoryBufferManager() {
  return gpu_channel_establish_factory_->GetGpuMemoryBufferManager();
}

cc::TaskGraphRunner* VizProcessTransportFactory::GetTaskGraphRunner() {
  return raster_thread_helper_.task_graph_runner();
}

const viz::ResourceSettings& VizProcessTransportFactory::GetResourceSettings()
    const {
  return renderer_settings_.resource_settings;
}

void VizProcessTransportFactory::AddObserver(
    ui::ContextFactoryObserver* observer) {
  observer_list_.AddObserver(observer);
}

void VizProcessTransportFactory::RemoveObserver(
    ui::ContextFactoryObserver* observer) {
  observer_list_.RemoveObserver(observer);
}

std::unique_ptr<ui::Reflector> VizProcessTransportFactory::CreateReflector(
    ui::Compositor* source,
    ui::Layer* target) {
  // TODO(crbug.com/601869): Reflector needs to be rewritten for viz.
  NOTREACHED();
  return nullptr;
}

void VizProcessTransportFactory::RemoveReflector(ui::Reflector* reflector) {
  // TODO(crbug.com/601869): Reflector needs to be rewritten for viz.
  NOTREACHED();
}

viz::FrameSinkId VizProcessTransportFactory::AllocateFrameSinkId() {
  return frame_sink_id_allocator_.NextFrameSinkId();
}

viz::HostFrameSinkManager*
VizProcessTransportFactory::GetHostFrameSinkManager() {
  return BrowserMainLoop::GetInstance()->host_frame_sink_manager();
}

void VizProcessTransportFactory::SetDisplayVisible(ui::Compositor* compositor,
                                                   bool visible) {
  auto iter = compositor_data_map_.find(compositor);
  if (iter == compositor_data_map_.end())
    return;
  iter->second.display_private->SetDisplayVisible(visible);
}

void VizProcessTransportFactory::ResizeDisplay(ui::Compositor* compositor,
                                               const gfx::Size& size) {
  // Do nothing and resize when a CompositorFrame with a new size arrives.
}

void VizProcessTransportFactory::SetDisplayColorSpace(
    ui::Compositor* compositor,
    const gfx::ColorSpace& blending_color_space,
    const gfx::ColorSpace& output_color_space) {
  auto iter = compositor_data_map_.find(compositor);
  if (iter == compositor_data_map_.end())
    return;
  iter->second.display_private->SetDisplayColorSpace(blending_color_space,
                                                     output_color_space);
}

void VizProcessTransportFactory::SetAuthoritativeVSyncInterval(
    ui::Compositor* compositor,
    base::TimeDelta interval) {
  // TODO(crbug.com/772524): Deal with vsync later.
  NOTIMPLEMENTED();
}

void VizProcessTransportFactory::SetDisplayVSyncParameters(
    ui::Compositor* compositor,
    base::TimeTicks timebase,
    base::TimeDelta interval) {
  // TODO(crbug.com/772524): Deal with vsync later.
  NOTIMPLEMENTED();
}

void VizProcessTransportFactory::IssueExternalBeginFrame(
    ui::Compositor* compositor,
    const viz::BeginFrameArgs& args) {
  // TODO(crbug.com/772524): Deal with vsync later.
  NOTIMPLEMENTED();
}

void VizProcessTransportFactory::SetOutputIsSecure(ui::Compositor* compositor,
                                                   bool secure) {
  auto iter = compositor_data_map_.find(compositor);
  if (iter == compositor_data_map_.end())
    return;
  iter->second.display_private->SetOutputIsSecure(secure);
}

viz::FrameSinkManagerImpl* VizProcessTransportFactory::GetFrameSinkManager() {
  // FrameSinkManagerImpl is in the gpu process, not the browser process.
  NOTREACHED();
  return nullptr;
}

ui::ContextFactory* VizProcessTransportFactory::GetContextFactory() {
  return this;
}

ui::ContextFactoryPrivate*
VizProcessTransportFactory::GetContextFactoryPrivate() {
  return this;
}

viz::GLHelper* VizProcessTransportFactory::GetGLHelper() {
  NOTREACHED();
  return nullptr;
}

void VizProcessTransportFactory::CreateLayerTreeFrameSinkForGpuChannel(
    base::WeakPtr<ui::Compositor> compositor_weak_ptr,
    scoped_refptr<gpu::GpuChannelHost> gpu_channel) {
  ui::Compositor* compositor = compositor_weak_ptr.get();
  if (!compositor)
    return;

  DCHECK_EQ(compositor_data_map_.count(compositor), 0u);

  scoped_refptr<viz::ContextProvider> context_provider =
      CreateContextProvider(std::move(gpu_channel));
  if (!context_provider->BindToCurrentThread()) {
    // TODO(danakj): Retry creating ContextProvider.
    NOTIMPLEMENTED();
    return;
  }

  // Create interfaces for a root CompositorFrameSink.
  viz::mojom::CompositorFrameSinkAssociatedPtrInfo sink_info;
  viz::mojom::CompositorFrameSinkAssociatedRequest sink_request =
      mojo::MakeRequest(&sink_info);
  viz::mojom::CompositorFrameSinkClientPtr client;
  viz::mojom::CompositorFrameSinkClientRequest client_request =
      mojo::MakeRequest(&client);
  viz::mojom::DisplayPrivateAssociatedPtr display_private;
  viz::mojom::DisplayPrivateAssociatedRequest display_private_request =
      mojo::MakeRequest(&display_private);

  // Creates the viz end of the root CompositorFrameSink.
  GetHostFrameSinkManager()->CreateRootCompositorFrameSink(
      compositor->frame_sink_id(), compositor->widget(), renderer_settings_,
      std::move(sink_request), std::move(client),
      std::move(display_private_request));

  // This is used with ContextFactoryPrivate later.
  compositor_data_map_[compositor].display_private = std::move(display_private);

  // Create LayerTreeFrameSink with the browser end of CompositorFrameSink.
  viz::ClientLayerTreeFrameSink::InitParams params;
  params.gpu_memory_buffer_manager = GetGpuMemoryBufferManager();
  params.pipes.compositor_frame_sink_associated_info = std::move(sink_info);
  params.pipes.client_request = std::move(client_request);
  params.local_surface_id_provider =
      std::make_unique<viz::DefaultLocalSurfaceIdProvider>();

  compositor->SetLayerTreeFrameSink(
      std::make_unique<viz::ClientLayerTreeFrameSink>(
          std::move(context_provider), nullptr /* worker_context_provider */,
          &params));
}

VizProcessTransportFactory::CompositorData::CompositorData() = default;
VizProcessTransportFactory::CompositorData::CompositorData(
    CompositorData&& other) = default;
VizProcessTransportFactory::CompositorData::~CompositorData() = default;
VizProcessTransportFactory::CompositorData&
VizProcessTransportFactory::CompositorData::operator=(CompositorData&& other) =
    default;

}  // namespace content
