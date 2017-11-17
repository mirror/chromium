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

GpuClient::GpuClient(int render_process_id)
    : render_process_id_(render_process_id), weak_factory_(this) {
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
  callback_.reset();
  if (!bindings_.empty())
    return;
  BrowserGpuMemoryBufferManager* gpu_memory_buffer_manager =
      BrowserGpuMemoryBufferManager::current();
  if (gpu_memory_buffer_manager)
    gpu_memory_buffer_manager->ProcessRemoved(render_process_id_);
}

void GpuClient::PreEstablishGpuChannel() {
  EstablishGpuChannelImpl(base::nullopt);
}

void GpuClient::OnEstablishGpuChannel(
    mojo::ScopedMessagePipeHandle channel_handle,
    const gpu::GPUInfo& gpu_info,
    const gpu::GpuFeatureInfo& gpu_feature_info,
    GpuProcessHost::EstablishChannelStatus status) {
  DCHECK_EQ(channel_handle.is_valid(),
            status == GpuProcessHost::EstablishChannelStatus::SUCCESS);
  gpu_channel_requested_ = false;
  base::Optional<EstablishGpuChannelCallback> callback;
  callback.swap(callback_);
  DCHECK(!callback_);

  if (status == GpuProcessHost::EstablishChannelStatus::GPU_HOST_INVALID) {
    // GPU process may have crashed or been killed. Try again.
    EstablishGpuChannelImpl(callback);
    return;
  }
  if (callback) {
    // A request is waiting.
    callback->Run(render_process_id_, std::move(channel_handle), gpu_info,
                  gpu_feature_info);
    return;
  }
  if (status == GpuProcessHost::EstablishChannelStatus::SUCCESS) {
    // This is the case we pre-establish a channel before a request arrives.
    // Cache the channel for a future request.
    channel_handle_ = std::move(channel_handle);
    gpu_info_ = gpu_info;
    gpu_feature_info_ = gpu_feature_info;
  }
}

void GpuClient::OnCreateGpuMemoryBuffer(
    const CreateGpuMemoryBufferCallback& callback,
    const gfx::GpuMemoryBufferHandle& handle) {
  callback.Run(handle);
}

void GpuClient::ClearCallback() {
  if (!callback_)
    return;
  OnEstablishGpuChannel(
      mojo::ScopedMessagePipeHandle(), gpu::GPUInfo(),
      gpu::GpuFeatureInfo(),
      GpuProcessHost::EstablishChannelStatus::GPU_ACCESS_DENIED);
  DCHECK(!callback_);
}

void GpuClient::EstablishGpuChannel(
    const EstablishGpuChannelCallback& callback) {
  EstablishGpuChannelImpl(callback);
}

void GpuClient::EstablishGpuChannelImpl(
    base::Optional<EstablishGpuChannelCallback> callback) {
  // At most one channel should be requested. So clear previous request first.
  ClearCallback();
  callback_.swap(callback);
  GpuProcessHost* host = GpuProcessHost::Get();
  if (!host) {
    ClearCallback();
    return;
  }
  if (channel_handle_.is_valid()) {
    // If a channel has been pre-established and cached, return it right away.
    OnEstablishGpuChannel(
        std::move(channel_handle_), gpu_info_, gpu_feature_info_,
        GpuProcessHost::EstablishChannelStatus::SUCCESS);
    return;
  }
  if (gpu_channel_requested_)
    return;
  gpu_channel_requested_ = true;
  bool preempts = false;
  bool allow_view_command_buffers = false;
  bool allow_real_time_streams = false;
  host->EstablishGpuChannel(
      render_process_id_,
      ChildProcessHostImpl::ChildProcessUniqueIdToTracingProcessId(
          render_process_id_),
      preempts, allow_view_command_buffers, allow_real_time_streams,
      base::Bind(
          &GpuClient::OnEstablishGpuChannel, weak_factory_.GetWeakPtr()));
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
          id, size, format, usage, render_process_id_,
          base::Bind(&GpuClient::OnCreateGpuMemoryBuffer,
                     weak_factory_.GetWeakPtr(), callback));
}

void GpuClient::DestroyGpuMemoryBuffer(gfx::GpuMemoryBufferId id,
                                       const gpu::SyncToken& sync_token) {
  DCHECK(BrowserGpuMemoryBufferManager::current());

  BrowserGpuMemoryBufferManager::current()->ChildProcessDeletedGpuMemoryBuffer(
      id, render_process_id_, sync_token);
}

}  // namespace content
