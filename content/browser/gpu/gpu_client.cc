// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/gpu/gpu_client.h"

#include "components/viz/host/server_gpu_memory_buffer_manager.h"
#include "content/browser/gpu/gpu_process_host.h"
#include "content/common/child_process_host_impl.h"
#include "content/public/browser/browser_thread.h"
#include "gpu/ipc/client/gpu_channel_host.h"
#include "gpu/ipc/client/gpu_memory_buffer_impl.h"
#include "gpu/ipc/client/gpu_memory_buffer_impl_shared_memory.h"

namespace content {

GpuClient::GpuClient(int render_process_id,
                     viz::ServerGpuMemoryBufferManager* gmb_manager)
    : render_process_id_(render_process_id),
      gmb_manager_(gmb_manager),
      weak_factory_(this) {
  DCHECK(gmb_manager_);
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
  gmb_manager_->DestroyAllGpuMemoryBufferForClient(render_process_id_);
}

void GpuClient::OnEstablishGpuChannel(
    const EstablishGpuChannelCallback& callback,
    const IPC::ChannelHandle& channel,
    const gpu::GPUInfo& gpu_info,
    GpuProcessHost::EstablishChannelStatus status) {
  mojo::ScopedMessagePipeHandle channel_handle;
  channel_handle.reset(channel.mojo_handle);
  callback.Run(render_process_id_, std::move(channel_handle), gpu_info);
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
        callback, IPC::ChannelHandle(), gpu::GPUInfo(),
        GpuProcessHost::EstablishChannelStatus::GPU_ACCESS_DENIED);
    return;
  }

  bool preempts = false;
  bool allow_view_command_buffers = false;
  bool allow_real_time_streams = false;
  host->EstablishGpuChannel(
      render_process_id_,
      ChildProcessHostImpl::ChildProcessUniqueIdToTracingProcessId(
          render_process_id_),
      preempts, allow_view_command_buffers, allow_real_time_streams,
      base::Bind(&GpuClient::OnEstablishGpuChannel, weak_factory_.GetWeakPtr(),
                 callback));
}

void GpuClient::CreateGpuMemoryBuffer(
    gfx::GpuMemoryBufferId id,
    const gfx::Size& size,
    gfx::BufferFormat format,
    gfx::BufferUsage usage,
    const ui::mojom::Gpu::CreateGpuMemoryBufferCallback& callback) {
  base::CheckedNumeric<int> bytes = size.width();
  bytes *= size.height();
  if (!bytes.IsValid()) {
    OnCreateGpuMemoryBuffer(callback, gfx::GpuMemoryBufferHandle());
    return;
  }

  gmb_manager_->AllocateGpuMemoryBuffer(
      id, render_process_id_, size, format, usage, gpu::kNullSurfaceHandle,
      base::Bind(&GpuClient::OnCreateGpuMemoryBuffer,
                 weak_factory_.GetWeakPtr(), callback));
}

void GpuClient::DestroyGpuMemoryBuffer(gfx::GpuMemoryBufferId id,
                                       const gpu::SyncToken& sync_token) {
  gmb_manager_->DestroyGpuMemoryBuffer(id, render_process_id_, sync_token);
}

}  // namespace content
