// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/compositor/viz_process_transport_factory.h"

#include "components/viz/client/client_layer_tree_frame_sink.h"
#include "components/viz/client/local_surface_id_provider.h"
#include "components/viz/common/gpu/context_provider.h"
#include "components/viz/host/host_frame_sink_manager.h"
#include "components/viz/host/renderer_settings_creation.h"
#include "content/browser/browser_main_loop.h"
#include "content/browser/browser_thread_impl.h"
#include "content/browser/gpu/browser_gpu_client.h"
#include "content/browser/gpu/compositor_util.h"
#include "content/browser/gpu/gpu_process_host.h"
#include "content/common/child_process_host_impl.h"
#include "content/public/common/content_client.h"
#include "content/public/common/service_manager_connection.h"
#include "services/service_manager/public/cpp/connector.h"
#include "services/ui/public/interfaces/gpu.mojom.h"
#include "ui/compositor/reflector.h"

namespace content {
namespace {

ui::mojom::GpuPtr BindBrowserGpuRequest() {
  // Creates |gpu_ptr| on UI thread, then hop to the IO thread to bind other end
  // of |gpu_ptr|.
  ui::mojom::GpuPtr gpu_ptr;
  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::BindOnce(
          [](ui::mojom::GpuRequest request) {
            BrowserGpuClient::Get()->BindGpuRequest(std::move(request));
          },
          MakeRequest(&gpu_ptr)));
  return gpu_ptr;
}

}  // namespace

VizProcessTransportFactory::VizProcessTransportFactory()
    : browser_client_id_(ChildProcessHostImpl::GenerateChildProcessUniqueId()),
      frame_sink_id_allocator_(browser_client_id_),
      renderer_settings_(
          viz::CreateRendererSettings(CreateBufferToTextureTargetMap())),
      weak_ptr_factory_(this) {
  // Create BrowserGpuClient on UI thread before it's needed. Note that
  // BrowserGpuClient can only be used or destroyed from IO thread afterwards.
  BrowserGpuClient::Create(browser_client_id_);
  gpu_ =
      ui::Gpu::Create(base::BindRepeating(&BindBrowserGpuRequest),
                      BrowserThread::GetTaskRunnerForThread(BrowserThread::IO));
}

VizProcessTransportFactory::~VizProcessTransportFactory() {
  // Destroy |gpu_| before destroying BrowserGpuClient.
  gpu_.reset();
  BrowserThread::PostTask(BrowserThread::IO, FROM_HERE, base::BindOnce([]() {
                            BrowserGpuClient::Destroy();
                          }));
}

void VizProcessTransportFactory::CreateLayerTreeFrameSink(
    base::WeakPtr<ui::Compositor> compositor) {
  gpu_->EstablishGpuChannel(base::Bind(
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
  return gpu_->gpu_memory_buffer_manager();
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
  // Do nothing and resize when the resized CompositorFrame arrives.
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
  // FrameSinkManagerImpl is in the viz process, not the browser process.
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

void VizProcessTransportFactory::SetGpuChannelEstablishFactory(
    gpu::GpuChannelEstablishFactory* factory) {
  NOTREACHED();
}

void VizProcessTransportFactory::CreateLayerTreeFrameSinkForGpuChannel(
    base::WeakPtr<ui::Compositor> compositor_weak_ptr,
    scoped_refptr<gpu::GpuChannelHost> gpu_channel) {
  ui::Compositor* compositor = compositor_weak_ptr.get();
  if (!compositor)
    return;

  DCHECK_EQ(compositor_data_map_.count(compositor), 0u);

  scoped_refptr<viz::ContextProvider> context_provider =
      gpu_->CreateContextProvider(std::move(gpu_channel));
  if (!context_provider->BindToCurrentThread()) {
    // TODO(danakj): Retry binding.
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

  // The DisplayPrivatePtr will be used in ContextFactoryPrivate methods.
  compositor_data_map_[compositor].display_private = std::move(display_private);

  // Create LayerTreeFrameSink with the browser end of CompositorFrameSink.
  viz::ClientLayerTreeFrameSink::InitParams params;
  params.gpu_memory_buffer_manager = gpu_->gpu_memory_buffer_manager();
  params.pipes.compositor_frame_sink_associated_info = std::move(sink_info);
  params.pipes.client_request = std::move(client_request);
  params.local_surface_id_provider =
      std::make_unique<viz::DefaultLocalSurfaceIdProvider>();

  auto layer_tree_frame_sink = std::make_unique<viz::ClientLayerTreeFrameSink>(
      std::move(context_provider), nullptr /* worker_context_provider */,
      &params);

  compositor->SetLayerTreeFrameSink(std::move(layer_tree_frame_sink));
}

VizProcessTransportFactory::CompositorData::CompositorData() = default;
VizProcessTransportFactory::CompositorData::CompositorData(
    CompositorData&& other) = default;
VizProcessTransportFactory::CompositorData::~CompositorData() = default;
VizProcessTransportFactory::CompositorData&
VizProcessTransportFactory::CompositorData::operator=(CompositorData&& other) =
    default;

}  // namespace content
