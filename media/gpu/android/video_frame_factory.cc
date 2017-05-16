// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/gpu/android/video_frame_factory.h"

#include "base/bind.h"
#include "base/callback.h"
#include "base/task_runner_util.h"
#include "gpu/ipc/service/gpu_channel.h"
#include "gpu/ipc/service/gpu_command_buffer_stub.h"
#include "media/base/video_frame.h"
#include "media/gpu/android/media_codec_wrapper.h"
#include "media/gpu/ipc/service/media_gpu_channel_manager.h"
#include "ui/gl/android/surface_texture.h"

namespace media {
namespace {

// Note: SurfaceTexture#detachFromGLContext() is buggy on a lot of devices so
// the returned SurfaceTexture is permanently attached to |gl_decoder|'s
// context. As a result this context must be made current before calling
// SurfaceTexture#updateTexImage().
scoped_refptr<SurfaceTextureGLOwner> CreateAttachedSurfaceTexture(
    gpu::gles2::GLES2Decoder* gl_decoder) {
  if (!gl_decoder->MakeCurrent())
    return nullptr;

  auto surface_texture = SurfaceTextureGLOwner::Create();
  if (!surface_texture)
    return nullptr;

  // Set the parameters on the texture.
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_EXTERNAL_OES, surface_texture->texture_id());
  glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  gl_decoder->RestoreTextureUnitBindings(0);
  gl_decoder->RestoreActiveTexture();
  DCHECK_EQ(static_cast<GLenum>(GL_NO_ERROR), glGetError());

  return surface_texture;
}

}  // namespace

VideoFrameFactory::VideoFrameFactory(
    scoped_refptr<base::SingleThreadTaskRunner> gpu_task_runner)
    : gpu_task_runner_(gpu_task_runner), vff_(new VFF(gpu_task_runner)) {}

VideoFrameFactory::~VideoFrameFactory() {
  gpu_task_runner_->DeleteSoon(FROM_HERE, vff_.release());
}

void VideoFrameFactory::Initialize(
    base::Callback<gpu::GpuCommandBufferStub*()> get_command_buffer_stub_cb,
    InitCb init_cb) {
  base::PostTaskAndReplyWithResult(
      gpu_task_runner_.get(), FROM_HERE,
      base::Bind(&VFF::Initialize, base::Unretained(vff_.get()),
                 get_command_buffer_stub_cb),
      init_cb);
}

void VideoFrameFactory::NewVideoFrame(
    MediaCodecWrapper* codec,
    int output_buffer,
    base::TimeDelta timestamp,
    base::Callback<void(const scoped_refptr<VideoFrame>&)> frame_created_cb) {
  base::PostTaskAndReplyWithResult(
      gpu_task_runner_.get(), FROM_HERE,
      base::Bind(&VFF::NewVideoFrame, base::Unretained(vff_.get()), codec,
                 output_buffer, timestamp),
      frame_created_cb);
}

VFF::VFF(scoped_refptr<base::SingleThreadTaskRunner> gpu_task_runner)
    : gpu_task_runner_(gpu_task_runner),
      client_task_runner_(base::ThreadTaskRunnerHandle::Get()) {}

VFF::~VFF() {}

scoped_refptr<SurfaceTextureGLOwner> VFF::Initialize(
    base::Callback<gpu::GpuCommandBufferStub*()> get_command_buffer_stub_cb) {
  auto stub = get_command_buffer_stub_cb.Run();
  if (!stub)
    return nullptr;

  gl_decoder_ = stub->decoder()->AsWeakPtr();
  if (!gl_decoder_)
    return nullptr;

  return CreateAttachedSurfaceTexture(gl_decoder_.get());
}

scoped_refptr<VideoFrame> VFF::NewVideoFrame(MediaCodecWrapper* codec,
                                             int output_buffer,
                                             base::TimeDelta timestamp) {
  if (!gl_decoder_)
    return nullptr;

  return VideoFrame::CreateZeroInitializedFrame(
      PIXEL_FORMAT_YV12, gfx::Size(1, 1), gfx::Rect(1, 1), gfx::Size(1, 1),
      timestamp);
}

}  // namespace media
