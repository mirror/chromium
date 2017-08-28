// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/input/target_frame_for_input_impl.h"

#include "base/bind.h"
#include "base/debug/stack_trace.h"
#include "base/logging.h"
#include "content/renderer/render_widget.h"

namespace content {

TargetFrameForInputImpl::TargetFrameForInputImpl(
    base::WeakPtr<RenderFrameImpl> render_frame,
    viz::mojom::TargetFrameForInputDelegateRequest request)
    : binding_(this),
      render_frame_(render_frame),
      input_event_queue_(render_frame->GetRenderWidget()->GetInputEventQueue()),
      main_thread_task_runner_(base::ThreadTaskRunnerHandle::Get()),
      weak_ptr_factory_(this)

{
  weak_this_ = weak_ptr_factory_.GetWeakPtr();
  // If we have created an input event queue move the mojo request over to the
  // compositor thread.
  if (RenderThreadImpl::current()->compositor_task_runner() &&
      input_event_queue_) {
    // Mojo channel bound on compositor thread.
    RenderThreadImpl::current()->compositor_task_runner()->PostTask(
        FROM_HERE, base::BindOnce(&TargetFrameForInputImpl::BindNow,
                                  base::Unretained(this), std::move(request)));
  } else {
    // Mojo channel bound on main thread.
    BindNow(std::move(request));
  }
}

TargetFrameForInputImpl::~TargetFrameForInputImpl() {}

// static
void TargetFrameForInputImpl::CreateMojoService(
    base::WeakPtr<RenderFrameImpl> render_frame,
    viz::mojom::TargetFrameForInputDelegateRequest request) {
  DCHECK(render_frame);

  // Owns itself. Will be deleted when message pipe is destroyed.
  new TargetFrameForInputImpl(render_frame, std::move(request));
}

void TargetFrameForInputImpl::RunOnMainThread(base::OnceClosure closure) {
  if (input_event_queue_) {
    input_event_queue_->QueueClosure(std::move(closure));
  } else {
    std::move(closure).Run();
  }
}

void TargetFrameForInputImpl::HitTestFrameAt(const gfx::Point& point,
                                             HitTestFrameAtCallback callback) {
  if (!main_thread_task_runner_->BelongsToCurrentThread()) {
    RunOnMainThread(
        base::BindOnce(&TargetFrameForInputImpl::HitTestOnMainThread,
                       weak_this_, point, std::move(callback)));
    return;
  }
  if (!render_frame_)
    return;
  HitTestOnMainThread(point, std::move(callback));
}

void TargetFrameForInputImpl::HitTestOnMainThread(
    const gfx::Point& point,
    HitTestFrameAtCallback callback) {
  std::move(callback).Run(
      render_frame_->GetRenderWidget()->HitTestFrameAt(point));
}

void TargetFrameForInputImpl::Release() {
  if (!main_thread_task_runner_->BelongsToCurrentThread()) {
    // Close the binding on the compositor thread first before telling the main
    // thread to delete this object.
    binding_.Close();
    main_thread_task_runner_->PostTask(
        FROM_HERE, base::Bind(&TargetFrameForInputImpl::Release, weak_this_));
    return;
  }
  delete this;
}

void TargetFrameForInputImpl::BindNow(
    viz::mojom::TargetFrameForInputDelegateRequest request) {
  binding_.Bind(std::move(request));
  binding_.set_connection_error_handler(
      base::Bind(&TargetFrameForInputImpl::Release, base::Unretained(this)));
}

}  // namespace content
