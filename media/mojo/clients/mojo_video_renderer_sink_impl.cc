// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/mojo/clients/mojo_video_renderer_sink_impl.h"

#include "media/mojo/common/media_type_converters.h"

namespace media {

MojoVideoRendererSinkImpl::MojoVideoRendererSinkImpl(
    const scoped_refptr<base::SingleThreadTaskRunner>& media_task_runner,
    media::VideoRendererSink* sink,
    mojo::InterfaceRequest<mojom::VideoRendererSink> request)
    : media_task_runner_(media_task_runner),
      compositor_task_runner_(nullptr),
      binding_(this, std::move(request)),
      sink_(sink),
      started_(false) {
  CHECK(media_task_runner_->BelongsToCurrentThread());
}

MojoVideoRendererSinkImpl::~MojoVideoRendererSinkImpl() {
  CHECK(media_task_runner_->BelongsToCurrentThread());

  if (started_) {
    sink_->Stop();
    started_ = false;
  }
}

scoped_refptr<VideoFrame> MojoVideoRendererSinkImpl::Render(
    base::TimeTicks deadline_min,
    base::TimeTicks deadline_max,
    bool background_rendering) {
  if (!callback_.is_bound()) {
    callback_.Bind(std::move(callback_info_));

    if (!compositor_task_runner_) {
      compositor_task_runner_ = base::ThreadTaskRunnerHandle::Get();
      CHECK(compositor_task_runner_ != media_task_runner_);
    }
  }

  CHECK(compositor_task_runner_->BelongsToCurrentThread());

  scoped_refptr<VideoFrame> frame;
  callback_->Render(deadline_min, deadline_max, background_rendering, &frame);

  // This ensures that the callback_ is reset in the compoistor thread.
  // It would be wrong to get it released as MojoVideoRendererSinkImpl is
  // destroyed because ~MojoVideoRendererSinkImpl is called from another thread,
  // i.e. the media thread.
  // An alternative would be to bind it to the media thread and use a
  // post/cond/wait/signal.
  callback_info_ = callback_.PassInterface();

  return frame;
}

void MojoVideoRendererSinkImpl::OnFrameDropped() {
  CHECK(compositor_task_runner_->BelongsToCurrentThread());

  if (!callback_.is_bound())
    callback_.Bind(std::move(callback_info_));

  callback_->OnFrameDropped();

  // Same reason than above.
  callback_info_ = callback_.PassInterface();
}

void MojoVideoRendererSinkImpl::Start(
    mojom::VideoRendererSinkRenderCallbackAssociatedPtrInfo callback) {
  CHECK(media_task_runner_->BelongsToCurrentThread());
  CHECK(!started_);
  callback_info_ = std::move(callback);
  started_ = true;

  sink_->Start(this);
}

void MojoVideoRendererSinkImpl::Stop() {
  CHECK(media_task_runner_->BelongsToCurrentThread());
  CHECK(started_);
  started_ = false;
  sink_->Stop();
}

void MojoVideoRendererSinkImpl::PaintSingleFrame(
    const scoped_refptr<VideoFrame>& frame,
    bool repaint_duplicate_frame) {
  CHECK(media_task_runner_->BelongsToCurrentThread());
  sink_->PaintSingleFrame(frame, repaint_duplicate_frame);
}

}  // namespace media
