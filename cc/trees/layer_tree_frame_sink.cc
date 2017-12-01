// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/trees/layer_tree_frame_sink.h"

#include <stdint.h>

#include "base/bind.h"
#include "base/location.h"
#include "base/macros.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/trace_event/trace_event.h"
#include "cc/trees/layer_tree_frame_sink_client.h"
#include "gpu/GLES2/gl2extchromium.h"
#include "gpu/command_buffer/client/context_support.h"
#include "gpu/command_buffer/client/gles2_interface.h"

namespace cc {

LayerTreeFrameSink::LayerTreeFrameSink(
    scoped_refptr<viz::ContextProvider> context_provider,
    scoped_refptr<viz::ContextProvider> worker_context_provider,
    gpu::GpuMemoryBufferManager* gpu_memory_buffer_manager,
    viz::SharedBitmapManager* shared_bitmap_manager)
    : context_provider_(std::move(context_provider)),
      worker_context_provider_(std::move(worker_context_provider)),
      gpu_memory_buffer_manager_(gpu_memory_buffer_manager),
      shared_bitmap_manager_(shared_bitmap_manager),
      weak_ptr_factory_(this) {}

LayerTreeFrameSink::LayerTreeFrameSink(
    scoped_refptr<viz::VulkanContextProvider> vulkan_context_provider)
    : vulkan_context_provider_(vulkan_context_provider),
      gpu_memory_buffer_manager_(nullptr),
      shared_bitmap_manager_(nullptr),
      weak_ptr_factory_(this) {}

LayerTreeFrameSink::~LayerTreeFrameSink() {
  if (client_)
    DetachFromClient();
}

bool LayerTreeFrameSink::BindToClient(LayerTreeFrameSinkClient* client) {
  DCHECK(client);
  DCHECK(!client_);

  client_ = client;
  task_runner_ = base::ThreadTaskRunnerHandle::Get();

  if (context_provider_) {
    auto result = context_provider_->BindToCurrentThread();
    if (result != gpu::ContextResult::kSuccess) {
      DetachFromClient();
      return false;
    }
    context_provider_->AddObserver(this);
  }

  if (worker_context_provider_) {
    viz::ContextProvider::ScopedContextLock lock(
        worker_context_provider_.get());
    if (worker_context_provider_->ContextGL()->GetGraphicsResetStatusKHR() !=
        GL_NO_ERROR) {
      DetachFromClient();
      return false;
    }
    worker_context_provider_->AddObserver(this);
  }

  return true;
}

void LayerTreeFrameSink::DetachFromClient() {
  DCHECK(client_);

  weak_ptr_factory_.InvalidateWeakPtrs();

  if (worker_context_provider_) {
    viz::ContextProvider::ScopedContextLock lock(
        worker_context_provider_.get());
    worker_context_provider_->RemoveObserver(this);
  }

  if (context_provider_)
    context_provider_->RemoveObserver(this);

  // Do not release worker context provider here as this is called on the
  // compositor thread and it must be released on the main thread. However,
  // compositor context provider must be released here.
  client_ = nullptr;
  task_runner_ = nullptr;
  context_provider_ = nullptr;
}

void LayerTreeFrameSink::OnContextLost() {
  TRACE_EVENT0("cc", "LayerTreeFrameSink::OnContextLost");
  // Worker context loss notification arrives on main thread, so post it to
  // compositor thread.
  if (task_runner_->BelongsToCurrentThread()) {
    client_->DidLoseLayerTreeFrameSink();
  } else {
    task_runner_->PostTask(FROM_HERE,
                           base::Bind(&LayerTreeFrameSink::OnContextLost,
                                      weak_ptr_factory_.GetWeakPtr()));
  }
}

}  // namespace cc
