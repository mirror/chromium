// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/mojo/services/mojo_video_renderer_sink_adapter.h"

#include "media/mojo/common/media_type_converters.h"

namespace media {

MojoVideoRendererSinkAdapter::MojoVideoRendererSinkAdapter(
    const scoped_refptr<base::SingleThreadTaskRunner>& task_runner)
    : binding_(this),
      task_runner_(task_runner),
      started_(false),
      render_callback_(nullptr) {}

MojoVideoRendererSinkAdapter::~MojoVideoRendererSinkAdapter() {}

void MojoVideoRendererSinkAdapter::Initialize(
    mojom::VideoRendererSinkPtr sink) {
  CHECK(sink);
  CHECK(!sink_);
  CHECK(task_runner_->BelongsToCurrentThread());
  sink_ = std::move(sink);
}

void MojoVideoRendererSinkAdapter::Start(
    media::VideoRendererSink::RenderCallback* callback) {
  CHECK(task_runner_->BelongsToCurrentThread());
  CHECK(sink_);
  CHECK(!started_);
  CHECK(callback);
  render_callback_ = callback;
  started_ = true;

  mojom::VideoRendererSinkRenderCallbackAssociatedPtrInfo
      render_callback_ptr_info;
  binding_.Bind(mojo::MakeRequest(&render_callback_ptr_info));

  sink_->Start(std::move(render_callback_ptr_info));
}

void MojoVideoRendererSinkAdapter::Stop() {
  CHECK(task_runner_->BelongsToCurrentThread());
  CHECK(started_);
  started_ = false;
  render_callback_ = nullptr;
  sink_->Stop();
}

void MojoVideoRendererSinkAdapter::PaintSingleFrame(
    const scoped_refptr<VideoFrame>& frame,
    bool repaint_duplicate_frame) {
  CHECK(task_runner_->BelongsToCurrentThread());
  CHECK(frame->storage_type() == media::VideoFrame::STORAGE_MOJO_SHARED_BUFFER);
  CHECK(sink_);
  sink_->PaintSingleFrame(frame, repaint_duplicate_frame);
}

void MojoVideoRendererSinkAdapter::Render(
    base::TimeTicks deadline_min,
    base::TimeTicks deadline_max,
    bool background_rendering,
    media::mojom::VideoRendererSinkRenderCallback::RenderCallback
        mojo_callback) {
  CHECK(task_runner_->BelongsToCurrentThread());

  if (!started_) {
    std::move(mojo_callback).Run(nullptr);
    return;
  }

  scoped_refptr<VideoFrame> frame = render_callback_->Render(
      deadline_min, deadline_max, background_rendering);
  CHECK(frame->storage_type() == media::VideoFrame::STORAGE_MOJO_SHARED_BUFFER);
  std::move(mojo_callback).Run(frame);
}

void MojoVideoRendererSinkAdapter::OnFrameDropped() {
  CHECK(task_runner_->BelongsToCurrentThread());
  render_callback_->OnFrameDropped();
}

}  // namespace media
