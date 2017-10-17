// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/public/cpp/gpu/gpu.h"

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

struct GpuChannelInfo {
  GpuChannelInfo() = default;
  ~GpuChannelInfo() = default;
  GpuChannelInfo(GpuChannelInfo&& other) = default;
  GpuChannelInfo& operator=(GpuChannelInfo&& other) = default;

  int client_id = 0;
  mojo::ScopedMessagePipeHandle channel_handle;
  gpu::GPUInfo gpu_info;
  gpu::GpuFeatureInfo gpu_feature_info;
};

Gpu::Gpu(GpuPtrFactory factory,
         scoped_refptr<base::SingleThreadTaskRunner> task_runner)
    : main_task_runner_(base::ThreadTaskRunnerHandle::Get()),
      io_task_runner_(std::move(task_runner)),
      sync_event_(base::WaitableEvent::ResetPolicy::MANUAL,
                  base::WaitableEvent::InitialState::SIGNALED),
      shutdown_event_(base::WaitableEvent::ResetPolicy::AUTOMATIC,
                      base::WaitableEvent::InitialState::NOT_SIGNALED) {
  TRACE_EVENT0("yyy", __FUNCTION__);

  DCHECK(main_task_runner_);
  gpu_memory_buffer_manager_ =
      std::make_unique<ClientGpuMemoryBufferManager>(factory.Run());
  if (!io_task_runner_) {
    io_thread_ = std::make_unique<base::Thread>("GPUIOThread");
    base::Thread::Options thread_options(base::MessageLoop::TYPE_IO, 0);
    thread_options.priority = base::ThreadPriority::NORMAL;
    CHECK(io_thread_->StartWithOptions(thread_options));
    io_task_runner_ = io_thread_->task_runner();
  }
  DCHECK(main_task_runner_ != io_task_runner_);
  gpu_ = mojom::ThreadSafeGpuPtr::Create(factory.Run().PassInterface(),
                                         io_task_runner_);
}

Gpu::~Gpu() {
  DCHECK(IsMainThread());
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

// static
std::unique_ptr<Gpu> Gpu::Create(
    GpuPtrFactory factory,
    scoped_refptr<base::SingleThreadTaskRunner> task_runner) {
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

void Gpu::CloseChannel() {
  DCHECK(IsMainThread());
  if (gpu_channel_) {
    gpu_channel_->DestroyChannel();
    gpu_channel_ = nullptr;
  }
}

void Gpu::CreateJpegDecodeAccelerator(
    media::mojom::GpuJpegDecodeAcceleratorRequest jda_request) {
  (*gpu_)->CreateJpegDecodeAccelerator(std::move(jda_request));
}

void Gpu::CreateVideoEncodeAcceleratorProvider(
    media::mojom::VideoEncodeAcceleratorProviderRequest vea_provider_request) {
  (*gpu_)->CreateVideoEncodeAcceleratorProvider(
      std::move(vea_provider_request));
}

void Gpu::EstablishGpuChannel(
    const gpu::GpuChannelEstablishedCallback& callback) {
  TRACE_EVENT0("yyy", __FUNCTION__);
  DCHECK(IsMainThread());
  if (GetGpuChannel()) {
    callback.Run(gpu_channel_);
    return;
  }

  const bool gpu_channel_request_ongoing = !establish_callbacks_.empty();
  // Cache |callback| but don't launch more than one EstablishGpuChannel().
  establish_callbacks_.push_back(callback);
  if (gpu_channel_request_ongoing)
    return;

  io_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&Gpu::EstablishGpuChannelOnIO, base::Unretained(this)));
}

scoped_refptr<gpu::GpuChannelHost> Gpu::EstablishGpuChannelSync() {
  TRACE_EVENT0("yyy", __FUNCTION__);
  DCHECK(IsMainThread());
  if (GetGpuChannel())
    return gpu_channel_;

  const bool gpu_channel_request_ongoing = !establish_callbacks_.empty();
  if (!gpu_channel_request_ongoing) {
    // Start EstablishGpuChannel task only if not already started.
    io_task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(&Gpu::EstablishGpuChannelOnIO, base::Unretained(this)));
  }

  // We need to handle both the
  sync_event_.Reset();
  {
    TRACE_EVENT0("yyy", "Gpu::EstablishGpuChannelSync");
    sync_event_.Wait();
  }
  OnEstablishedGpuChannel();

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

void Gpu::OnEstablishedGpuChannel() {
  TRACE_EVENT0("yyy", __FUNCTION__);
  DCHECK(IsMainThread());
  DCHECK(!gpu_channel_);
  DCHECK(gpu_channel_info_);

  if (gpu_channel_info_->client_id &&
      gpu_channel_info_->channel_handle.is_valid()) {
    gpu_channel_ = gpu::GpuChannelHost::Create(
        this, gpu_channel_info_->client_id, gpu_channel_info_->gpu_info,
        gpu_channel_info_->gpu_feature_info,
        IPC::ChannelHandle(gpu_channel_info_->channel_handle.release()),
        &shutdown_event_, gpu_memory_buffer_manager_.get());
  }
  gpu_channel_info_.reset();

  auto callbacks = std::move(establish_callbacks_);
  establish_callbacks_.clear();
  for (const auto& callback : callbacks)
    callback.Run(gpu_channel_);
}

void Gpu::EstablishGpuChannelOnIO() {
  TRACE_EVENT0("yyy", __FUNCTION__);
  DCHECK(IsIOThread());
  // Response from EstablishGpuChannel() must happen on IO thread to allow
  // for signal in async mode.
  (*gpu_)->EstablishGpuChannel(
      base::Bind(&Gpu::OnEstablishedGpuChannelOnIO, base::Unretained(this)));
}

void Gpu::OnEstablishedGpuChannelOnIO(
    int client_id,
    mojo::ScopedMessagePipeHandle channel_handle,
    const gpu::GPUInfo& gpu_info,
    const gpu::GpuFeatureInfo& gpu_feature_info) {
  TRACE_EVENT0("yyy", __FUNCTION__);
  DCHECK(IsIOThread());

  DCHECK(!gpu_channel_info_);
  gpu_channel_info_ = std::make_unique<GpuChannelInfo>();
  gpu_channel_info_->client_id = client_id;
  gpu_channel_info_->channel_handle = std::move(channel_handle);
  gpu_channel_info_->gpu_info = gpu_info;
  gpu_channel_info_->gpu_feature_info = gpu_feature_info;

  // The sync path will continue immediately on the main thread while the async
  // path will PostTask to the main thread.
  if (!sync_event_.IsSignaled()) {
    sync_event_.Signal();
  } else {
    main_task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(&Gpu::OnEstablishedGpuChannel, base::Unretained(this)));
  }
}

bool Gpu::IsIOThread() {
  return io_task_runner_->BelongsToCurrentThread();
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
