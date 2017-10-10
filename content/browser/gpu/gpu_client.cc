// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/gpu/gpu_client.h"

#include "content/browser/gpu/browser_gpu_memory_buffer_manager.h"
#include "content/browser/gpu/gpu_process_host.h"
#include "content/common/child_process_host_impl.h"
#include "content/public/browser/browser_thread.h"
#include "gpu/ipc/client/gpu_channel_host.h"
#include "gpu/ipc/client/gpu_memory_buffer_impl.h"
#include "gpu/ipc/client/gpu_memory_buffer_impl_shared_memory.h"

namespace content {

GpuClient::GpuClient(int client_process_id, bool is_privileged_client)
    : client_process_id_(client_process_id),
      is_privileged_client_(is_privileged_client),
      weak_factory_(this) {
  bindings_.set_connection_error_handler(
      base::Bind(&GpuClient::OnError, base::Unretained(this)));
}

GpuClient::~GpuClient() {
  bindings_.CloseAllBindings();
  OnError();
}

void GpuClient::Add(ui::mojom::GpuRequest request) {
  bindings_.AddBinding(this, std::move(request));
}

void GpuClient::OnError() {
  if (!bindings_.empty())
    return;
  BrowserGpuMemoryBufferManager* gpu_memory_buffer_manager =
      BrowserGpuMemoryBufferManager::current();
  if (gpu_memory_buffer_manager)
    gpu_memory_buffer_manager->ProcessRemoved(client_process_id_);
}

void GpuClient::OnEstablishGpuChannel(
    const EstablishGpuChannelCallback& callback,
    const IPC::ChannelHandle& channel,
    const gpu::GPUInfo& gpu_info,
    const gpu::GpuFeatureInfo& gpu_feature_info,
    GpuProcessHost::EstablishChannelStatus status) {
  if (status == GpuProcessHost::EstablishChannelStatus::GPU_ACCESS_DENIED) {
    // GPU access is not allowed. Notify the client immediately.
    DCHECK(!channel.mojo_handle.is_valid());
    callback.Run(client_process_id_, mojo::ScopedMessagePipeHandle(), gpu_info,
                 gpu_feature_info);
    return;
  }

  if (status == GpuProcessHost::EstablishChannelStatus::GPU_HOST_INVALID) {
    // GPU process may have crashed or been killed. Try again.
    DCHECK(!channel.mojo_handle.is_valid());
    EstablishGpuChannel(callback);
    return;
  }
  DCHECK(channel.mojo_handle.is_valid());
  mojo::ScopedMessagePipeHandle channel_handle;
  channel_handle.reset(channel.mojo_handle);
  callback.Run(client_process_id_, std::move(channel_handle), gpu_info,
               gpu_feature_info);
}

void GpuClient::OnCreateGpuMemoryBuffer(
    const CreateGpuMemoryBufferCallback& callback,
    const gfx::GpuMemoryBufferHandle& handle) {
  callback.Run(handle);
}

void GpuClient::EstablishGpuChannel(
    const EstablishGpuChannelCallback& callback) {
  GpuProcessHost* host = GpuProcessHost::Get();
  if (!host) {
    OnEstablishGpuChannel(
        callback, IPC::ChannelHandle(), gpu::GPUInfo(), gpu::GpuFeatureInfo(),
        GpuProcessHost::EstablishChannelStatus::GPU_ACCESS_DENIED);
    return;
  }

  bool preempts = is_privileged_client_;
  bool allow_view_command_buffers = is_privileged_client_;
  bool allow_real_time_streams = is_privileged_client_;
  host->EstablishGpuChannel(
      client_process_id_,
      ChildProcessHostImpl::ChildProcessUniqueIdToTracingProcessId(
          client_process_id_),
      preempts, allow_view_command_buffers, allow_real_time_streams,
      base::Bind(&GpuClient::OnEstablishGpuChannel, weak_factory_.GetWeakPtr(),
                 callback));
}

void GpuClient::CreateJpegDecodeAccelerator(
    media::mojom::GpuJpegDecodeAcceleratorRequest jda_request) {
  GpuProcessHost* host = GpuProcessHost::Get();
  if (host)
    host->gpu_service()->CreateJpegDecodeAccelerator(std::move(jda_request));
}

void GpuClient::CreateVideoEncodeAcceleratorProvider(
    media::mojom::VideoEncodeAcceleratorProviderRequest vea_provider_request) {
  GpuProcessHost* host = GpuProcessHost::Get();
  if (!host)
    return;
  host->gpu_service()->CreateVideoEncodeAcceleratorProvider(
      std::move(vea_provider_request));
}

void GpuClient::CreateGpuMemoryBuffer(
    gfx::GpuMemoryBufferId id,
    const gfx::Size& size,
    gfx::BufferFormat format,
    gfx::BufferUsage usage,
    const ui::mojom::Gpu::CreateGpuMemoryBufferCallback& callback) {
  DCHECK(BrowserGpuMemoryBufferManager::current());

  base::CheckedNumeric<int> bytes = size.width();
  bytes *= size.height();
  if (!bytes.IsValid()) {
    OnCreateGpuMemoryBuffer(callback, gfx::GpuMemoryBufferHandle());
    return;
  }

  BrowserGpuMemoryBufferManager::current()
      ->AllocateGpuMemoryBufferForChildProcess(
          id, size, format, usage, client_process_id_,
          base::Bind(&GpuClient::OnCreateGpuMemoryBuffer,
                     weak_factory_.GetWeakPtr(), callback));
}

void GpuClient::DestroyGpuMemoryBuffer(gfx::GpuMemoryBufferId id,
                                       const gpu::SyncToken& sync_token) {
  DCHECK(BrowserGpuMemoryBufferManager::current());

  BrowserGpuMemoryBufferManager::current()->ChildProcessDeletedGpuMemoryBuffer(
      id, client_process_id_, sync_token);
}

}  // namespace content
