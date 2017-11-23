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

  if (!context_provider_)
    return false;

  client_ = client;

  context_provider_->AddObserver(this);

  auto result = context_provider_->BindToCurrentThread();
  if (result != gpu::ContextResult::kSuccess) {
    context_provider_ = nullptr;
    client_ = nullptr;
    return false;
  }

  return true;
}

void LayerTreeFrameSink::DetachFromClient() {
  DCHECK(client_);

  weak_ptr_factory_.InvalidateWeakPtrs();

  if (context_provider_)
    context_provider_->RemoveObserver(this);

  context_provider_ = nullptr;
  client_ = nullptr;
}

base::WeakPtr<LayerTreeFrameSink> LayerTreeFrameSink::AsWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

void LayerTreeFrameSink::OnWorkerContextLost() {
  TRACE_EVENT0("cc", "LayerTreeFrameSink::OnWorkerContextLost");
  client_->DidLoseLayerTreeFrameSink();
}

void LayerTreeFrameSink::OnContextLost() {
  TRACE_EVENT0("cc", "LayerTreeFrameSink::OnContextLost");
  client_->DidLoseLayerTreeFrameSink();
}

}  // namespace cc
