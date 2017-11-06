// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/public/cpp/gpu/gpu.h"

#include "base/debug/stack_trace.h"
#include "base/memory/ptr_util.h"
#include "base/message_loop/message_loop.h"
#include "base/threading/thread_task_runner_handle.h"
#include "build/build_config.h"
#include "gpu/command_buffer/common/scheduling_priority.h"
#include "mojo/public/cpp/bindings/sync_call_restrictions.h"
#include "mojo/public/cpp/system/platform_handle.h"
#include "services/service_manager/public/cpp/connector.h"
#include "services/ui/public/cpp/gpu/client_gpu_memory_buffer_manager.h"
#include "services/ui/public/cpp/gpu/context_provider_command_buffer.h"
#include "services/ui/public/interfaces/constants.mojom.h"
#include "services/ui/public/interfaces/gpu.mojom.h"

namespace ui {

namespace {

mojom::GpuPtr DefaultFactory(service_manager::Connector* connector,
                             const std::string& service_name) {
  mojom::GpuPtr gpu_ptr;
  connector->BindInterface(service_name, &gpu_ptr);
  return gpu_ptr;
}

}  // namespace

class Gpu::EstablishRequest
    : public base::RefCountedThreadSafe<Gpu::EstablishRequest> {
 public:
  EstablishRequest(Gpu* parent,
                   scoped_refptr<base::SingleThreadTaskRunner> main_task_runner)
      : parent_(parent), main_task_runner_(main_task_runner) {}

  int client_id() { return client_id_; }
  mojo::ScopedMessagePipeHandle& channel_handle() { return channel_handle_; }
  gpu::GPUInfo& gpu_info() { return gpu_info_; }
  gpu::GpuFeatureInfo& gpu_feature_info() { return gpu_feature_info_; }

  void SetWaitableEvent(base::WaitableEvent* establish_event) {
    DCHECK(main_task_runner_->BelongsToCurrentThread());
    base::AutoLock mutex(lock_);

    // Check if we've already received a response and are waiting for task
    // to run on main thread. Don't wait in that case.
    if (received_)
      return;

    establish_event_ = establish_event;
    establish_event_->Reset();
  }

  void SendRequest(scoped_refptr<ui::mojom::ThreadSafeGpuPtr> gpu) {
    DCHECK(!main_task_runner_->BelongsToCurrentThread());
    base::AutoLock autolock(lock_);

    if (finished_)
      return;

    (*gpu)->EstablishGpuChannel(
        base::Bind(&EstablishRequest::OnEstablishedGpuChannel, this));
  }

  void FinishOnMain() {
    // No lock needed, everything will run on |main_task_runner_| now.
    DCHECK(main_task_runner_->BelongsToCurrentThread());
    DCHECK(received_);

    if (finished_)
      return;

    finished_ = true;
    parent_->OnEstablishedGpuChannel();
  }

  void Cancel() {
    DCHECK(main_task_runner_->BelongsToCurrentThread());
    base::AutoLock autolock(lock_);
    DCHECK(!finished_);
    finished_ = true;
  }

 protected:
  friend class base::RefCountedThreadSafe<Gpu::EstablishRequest>;

  virtual ~EstablishRequest() = default;

  void OnEstablishedGpuChannel(int client_id,
                               mojo::ScopedMessagePipeHandle channel_handle,
                               const gpu::GPUInfo& gpu_info,
                               const gpu::GpuFeatureInfo& gpu_feature_info) {
    DCHECK(!main_task_runner_->BelongsToCurrentThread());
    base::AutoLock autolock(lock_);

    if (finished_)
      return;

    client_id_ = client_id;
    channel_handle_ = std::move(channel_handle);
    gpu_info_ = gpu_info;
    gpu_feature_info_ = gpu_feature_info;
    received_ = true;

    if (establish_event_) {
      establish_event_->Signal();
    } else {
      main_task_runner_->PostTask(
          FROM_HERE, base::Bind(&EstablishRequest::FinishOnMain, this));
    }
  }

  Gpu* const parent_;
  scoped_refptr<base::SingleThreadTaskRunner> main_task_runner_;
  base::WaitableEvent* establish_event_ = nullptr;

  base::Lock lock_;
  bool received_ = false;
  bool finished_ = false;

  int client_id_;
  mojo::ScopedMessagePipeHandle channel_handle_;
  gpu::GPUInfo gpu_info_;
  gpu::GpuFeatureInfo gpu_feature_info_;

  DISALLOW_COPY_AND_ASSIGN(EstablishRequest);
};

Gpu::Gpu(GpuPtrFactory factory,
         scoped_refptr<base::SingleThreadTaskRunner> task_runner)
    : main_task_runner_(base::ThreadTaskRunnerHandle::Get()),
      io_task_runner_(std::move(task_runner)),
      shutdown_event_(base::WaitableEvent::ResetPolicy::AUTOMATIC,
                      base::WaitableEvent::InitialState::NOT_SIGNALED) {
  DCHECK(main_task_runner_);
  DCHECK(io_task_runner_);

  gpu_ptr_ = mojom::ThreadSafeGpuPtr::Create(factory.Run().PassInterface(),
                                             io_task_runner_);
  gpu_memory_buffer_manager_ =
      std::make_unique<ClientGpuMemoryBufferManager>(factory.Run());
}

Gpu::~Gpu() {
  DCHECK(IsMainThread());
  if (request_) {
    request_->Cancel();
    request_ = nullptr;
  }
  gpu_ptr_ = nullptr;
  shutdown_event_.Signal();
  if (gpu_channel_)
    gpu_channel_->DestroyChannel();
}

// static
std::unique_ptr<Gpu> Gpu::Create(
    service_manager::Connector* connector,
    const std::string& service_name,
    scoped_refptr<base::SingleThreadTaskRunner> task_runner) {
  GpuPtrFactory factory =
      base::BindRepeating(&DefaultFactory, connector, service_name);
  return base::WrapUnique(new Gpu(std::move(factory), std::move(task_runner)));
}

scoped_refptr<viz::ContextProvider> Gpu::CreateContextProvider(
    scoped_refptr<gpu::GpuChannelHost> gpu_channel) {
  int32_t stream_id = 0;
  gpu::SchedulingPriority stream_priority = gpu::SchedulingPriority::kNormal;

  constexpr bool automatic_flushes = false;
  constexpr bool support_locking = false;

  gpu::gles2::ContextCreationAttribHelper attributes;
  attributes.alpha_size = -1;
  attributes.depth_size = 0;
  attributes.stencil_size = 0;
  attributes.samples = 0;
  attributes.sample_buffers = 0;
  attributes.bind_generates_resource = false;
  attributes.lose_context_when_out_of_memory = true;
  ui::ContextProviderCommandBuffer* shared_context_provider = nullptr;
  return base::MakeRefCounted<ui::ContextProviderCommandBuffer>(
      std::move(gpu_channel), stream_id, stream_priority,
      gpu::kNullSurfaceHandle, GURL("chrome://gpu/MusContextFactory"),
      automatic_flushes, support_locking, gpu::SharedMemoryLimits(), attributes,
      shared_context_provider, ui::command_buffer_metrics::MUS_CLIENT_CONTEXT);
}

void Gpu::CreateJpegDecodeAccelerator(
    media::mojom::GpuJpegDecodeAcceleratorRequest jda_request) {
  DCHECK(IsMainThread());
  (*gpu_ptr_)->CreateJpegDecodeAccelerator(std::move(jda_request));
}

void Gpu::CreateVideoEncodeAcceleratorProvider(
    media::mojom::VideoEncodeAcceleratorProviderRequest vea_provider_request) {
  DCHECK(IsMainThread());
  (*gpu_ptr_)->CreateVideoEncodeAcceleratorProvider(
      std::move(vea_provider_request));
}

void Gpu::EstablishGpuChannel(
    const gpu::GpuChannelEstablishedCallback& callback) {
  DCHECK(IsMainThread());
  scoped_refptr<gpu::GpuChannelHost> channel = GetGpuChannel();
  if (channel) {
    callback.Run(std::move(channel));
    return;
  }

  establish_callbacks_.push_back(callback);
  SendRequest();
}

scoped_refptr<gpu::GpuChannelHost> Gpu::EstablishGpuChannelSync(
    bool* connection_error) {
  DCHECK(IsMainThread());
  if (connection_error)
    *connection_error = false;

  scoped_refptr<gpu::GpuChannelHost> channel = GetGpuChannel();
  if (channel)
    return channel;

  SendRequest();
  base::WaitableEvent event(base::WaitableEvent::ResetPolicy::MANUAL,
                            base::WaitableEvent::InitialState::SIGNALED);
  request_->SetWaitableEvent(&event);
  event.Wait();
  request_->FinishOnMain();

  return gpu_channel_;
}

gpu::GpuMemoryBufferManager* Gpu::GetGpuMemoryBufferManager() {
  return gpu_memory_buffer_manager_.get();
}

scoped_refptr<gpu::GpuChannelHost> Gpu::GetGpuChannel() {
  DCHECK(IsMainThread());
  if (gpu_channel_ && gpu_channel_->IsLost()) {
    gpu_channel_->DestroyChannel();
    gpu_channel_ = nullptr;
  }
  return gpu_channel_;
}

void Gpu::SendRequest() {
  if (request_)
    return;

  request_ = base::MakeRefCounted<EstablishRequest>(this, main_task_runner_);
  io_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&EstablishRequest::SendRequest, request_, gpu_ptr_));
}

void Gpu::OnEstablishedGpuChannel() {
  DCHECK(IsMainThread());
  DCHECK(request_);
  DCHECK(!gpu_channel_);

  if (request_->client_id() && request_->channel_handle().is_valid()) {
    gpu_channel_ = gpu::GpuChannelHost::Create(
        this, request_->client_id(), request_->gpu_info(),
        request_->gpu_feature_info(),
        IPC::ChannelHandle(request_->channel_handle().release()),
        &shutdown_event_, gpu_memory_buffer_manager_.get());
  }
  request_ = nullptr;

  auto callbacks = std::move(establish_callbacks_);
  establish_callbacks_.clear();
  for (const auto& callback : callbacks)
    callback.Run(gpu_channel_);
}

bool Gpu::IsMainThread() {
  return main_task_runner_->BelongsToCurrentThread();
}

scoped_refptr<base::SingleThreadTaskRunner> Gpu::GetIOThreadTaskRunner() {
  return io_task_runner_;
}

std::unique_ptr<base::SharedMemory> Gpu::AllocateSharedMemory(size_t size) {
  mojo::ScopedSharedBufferHandle handle =
      mojo::SharedBufferHandle::Create(size);
  if (!handle.is_valid())
    return nullptr;

  base::SharedMemoryHandle platform_handle;
  size_t shared_memory_size;
  bool readonly;
  MojoResult result = mojo::UnwrapSharedMemoryHandle(
      std::move(handle), &platform_handle, &shared_memory_size, &readonly);
  if (result != MOJO_RESULT_OK)
    return nullptr;
  DCHECK_EQ(shared_memory_size, size);

  return base::MakeUnique<base::SharedMemory>(platform_handle, readonly);
}

}  // namespace ui
