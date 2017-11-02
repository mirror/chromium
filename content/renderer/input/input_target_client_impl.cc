// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/input/input_target_client_impl.h"

#include "base/bind.h"
#include "base/debug/stack_trace.h"
#include "base/logging.h"
#include "content/renderer/render_widget.h"

namespace content {

InputTargetClientImpl::InputTargetClientImpl(
    base::WeakPtr<RenderFrameImpl> render_frame,
    viz::mojom::InputTargetClientRequest request)
    : binding_(this),
      render_frame_(render_frame),
      input_event_queue_(render_frame->GetRenderWidget()->GetInputEventQueue()),
      main_thread_task_runner_(base::ThreadTaskRunnerHandle::Get()),
      weak_ptr_factory_(this)

{
  weak_this_ = weak_ptr_factory_.GetWeakPtr();
  BindNow(std::move(request));
}

InputTargetClientImpl::~InputTargetClientImpl() {}

// static
void InputTargetClientImpl::CreateMojoService(
    base::WeakPtr<RenderFrameImpl> render_frame,
    viz::mojom::InputTargetClientRequest request) {
  DCHECK(render_frame);

  // Owns itself. Will be deleted when message pipe is destroyed.
  new InputTargetClientImpl(render_frame, std::move(request));
}

void InputTargetClientImpl::RunOnMainThread(base::OnceClosure closure) {
  if (input_event_queue_) {
    input_event_queue_->QueueClosure(std::move(closure));
  } else {
    std::move(closure).Run();
  }
}

void InputTargetClientImpl::WidgetRoutingIdAt(
    const gfx::Point& point,
    WidgetRoutingIdAtCallback callback) {
  if (!main_thread_task_runner_->BelongsToCurrentThread()) {
    RunOnMainThread(base::BindOnce(&InputTargetClientImpl::HitTestOnMainThread,
                                   weak_this_, point, std::move(callback)));
    return;
  }
  if (!render_frame_)
    return;
  HitTestOnMainThread(point, std::move(callback));
}

void InputTargetClientImpl::HitTestOnMainThread(
    const gfx::Point& point,
    WidgetRoutingIdAtCallback callback) {
  std::move(callback).Run(
      render_frame_->GetRenderWidget()->GetWidgetRoutingIdAtPoint(point));
}

void InputTargetClientImpl::Release() {
  if (!main_thread_task_runner_->BelongsToCurrentThread()) {
    // Close the binding on the compositor thread first before telling the main
    // thread to delete this object.
    binding_.Close();
    main_thread_task_runner_->PostTask(
        FROM_HERE, base::Bind(&InputTargetClientImpl::Release, weak_this_));
    return;
  }
  delete this;
}

void InputTargetClientImpl::BindNow(
    viz::mojom::InputTargetClientRequest request) {
  binding_.Bind(std::move(request));
  binding_.set_connection_error_handler(
      base::Bind(&InputTargetClientImpl::Release, base::Unretained(this)));
}

}  // namespace content
