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
#include "content/browser/gpu/compositor_util.h"
#include "content/public/common/content_client.h"
#include "content/public/common/service_manager_connection.h"
#include "services/service_manager/public/cpp/connector.h"
#include "services/ui/public/interfaces/constants.mojom.h"
#include "ui/compositor/reflector.h"

namespace content {
namespace {

void DisplayPrivateError(viz::FrameSinkId frame_sink_id) {
  LOG(ERROR) << "viz::mojom::DisplayPrivate connection error for "
             << frame_sink_id;
}

}  // namespace

VizProcessTransportFactory::VizProcessTransportFactory()
    : frame_sink_id_allocator_(0),
      renderer_settings_(
          viz::CreateRendererSettings(CreateBufferToTextureTargetMap())),
      weak_ptr_factory_(this) {
  auto* service_manager_connection = ServiceManagerConnection::GetForProcess();
  auto* connector = service_manager_connection->GetConnector();
  gpu_ =
      ui::Gpu::Create(connector, "embedded_ui",
                      BrowserThread::GetTaskRunnerForThread(BrowserThread::IO));
}

VizProcessTransportFactory::~VizProcessTransportFactory() = default;

void VizProcessTransportFactory::CreateLayerTreeFrameSink(
    base::WeakPtr<ui::Compositor> compositor) {
  gpu_->EstablishGpuChannel(
      base::Bind(&VizProcessTransportFactory::OnEstablishedGpuChannel,
                 weak_ptr_factory_.GetWeakPtr(), compositor));
}

scoped_refptr<viz::ContextProvider>
VizProcessTransportFactory::SharedMainThreadContextProvider() {
  return nullptr;
}

void VizProcessTransportFactory::RemoveCompositor(ui::Compositor* compositor) {
  NOTIMPLEMENTED();
}

double VizProcessTransportFactory::GetRefreshRate() const {
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
    ui::ContextFactoryObserver* observer) {}

void VizProcessTransportFactory::RemoveObserver(
    ui::ContextFactoryObserver* observer) {}

std::unique_ptr<ui::Reflector> VizProcessTransportFactory::CreateReflector(
    ui::Compositor* source,
    ui::Layer* target) {
  NOTREACHED();
  return nullptr;
}

void VizProcessTransportFactory::RemoveReflector(ui::Reflector* reflector) {
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
  auto iter = compositor_data_map_.find(compositor);
  if (iter == compositor_data_map_.end())
    return;
  iter->second.display_private->ResizeDisplay(size);
}

void VizProcessTransportFactory::SetDisplayColorSpace(
    ui::Compositor* compositor,
    const gfx::ColorSpace& blending_color_space,
    const gfx::ColorSpace& output_color_space) {
  auto iter = compositor_data_map_.find(compositor);
  if (iter == compositor_data_map_.end())
    return;
  iter->second.display_private->SetDisplayColorSpace(output_color_space);
}

void VizProcessTransportFactory::SetAuthoritativeVSyncInterval(
    ui::Compositor* compositor,
    base::TimeDelta interval) {
  NOTIMPLEMENTED();
}

void VizProcessTransportFactory::SetDisplayVSyncParameters(
    ui::Compositor* compositor,
    base::TimeTicks timebase,
    base::TimeDelta interval) {
  NOTIMPLEMENTED();
}

void VizProcessTransportFactory::IssueExternalBeginFrame(
    ui::Compositor* compositor,
    const viz::BeginFrameArgs& args) {
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

void VizProcessTransportFactory::OnEstablishedGpuChannel(
    base::WeakPtr<ui::Compositor> compositor,
    scoped_refptr<gpu::GpuChannelHost> gpu_channel) {
  if (!compositor)
    return;

  scoped_refptr<viz::ContextProvider> context_provider =
      gpu_->CreateContextProvider(std::move(gpu_channel));
  // If the binding fails, then we need to return early since the compositor
  // expects a successfully initialized/bound provider.
  if (!context_provider->BindToCurrentThread()) {
    LOG(ERROR) << "Failure to bind context_provider_";
    return;
  }

  gpu::SurfaceHandle surface_handle = compositor->widget();

  viz::mojom::CompositorFrameSinkAssociatedPtrInfo sink_info;
  viz::mojom::CompositorFrameSinkAssociatedRequest sink_request =
      mojo::MakeRequest(&sink_info);
  viz::mojom::CompositorFrameSinkClientPtr client;
  viz::mojom::CompositorFrameSinkClientRequest client_request =
      mojo::MakeRequest(&client);
  viz::mojom::DisplayPrivateAssociatedPtr display_private;
  viz::mojom::DisplayPrivateAssociatedRequest display_private_request =
      mojo::MakeRequest(&display_private);

  viz::ClientLayerTreeFrameSink::InitParams params;
  params.gpu_memory_buffer_manager = gpu_->gpu_memory_buffer_manager();
  params.compositor_frame_sink_associated_info = std::move(sink_info);
  params.client_request = std::move(client_request);
  params.local_surface_id_provider =
      base::MakeUnique<viz::DefaultLocalSurfaceIdProvider>();

  auto layer_tree_frame_sink = std::make_unique<viz::ClientLayerTreeFrameSink>(
      std::move(context_provider), nullptr /* worker_context_provider */,
      &params);

  GetHostFrameSinkManager()->CreateRootCompositorFrameSink(
      compositor->frame_sink_id(), surface_handle, renderer_settings_,
      std::move(sink_request), std::move(client),
      std::move(display_private_request));

  display_private.set_connection_error_handler(
      base::Bind(&DisplayPrivateError, compositor->frame_sink_id()));
  compositor_data_map_[compositor.get()].display_private =
      std::move(display_private);

  compositor->SetLayerTreeFrameSink(std::move(layer_tree_frame_sink));
}

VizProcessTransportFactory::CompositorData::CompositorData() = default;
VizProcessTransportFactory::CompositorData::~CompositorData() = default;

}  // namespace content
