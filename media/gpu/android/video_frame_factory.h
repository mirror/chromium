// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_GPU_ANDROID_VIDEO_FRAME_FACTORY_
#define MEDIA_GPU_ANDROID_VIDEO_FRAME_FACTORY_

#include <tuple>

#include "gpu/command_buffer/service/gles2_cmd_decoder.h"
#include "gpu/ipc/service/gpu_command_buffer_stub.h"
#include "media/base/video_frame.h"
#include "media/gpu/android/media_codec_wrapper.h"
#include "media/gpu/ipc/service/media_gpu_channel_manager.h"
#include "media/gpu/surface_texture_gl_owner.h"
#include "ui/gl/gl_bindings.h"

namespace media {

class VFF;

// VideoFrameFactory is single threaded. It delegates to VFF for gpu thread
// operations.
class VideoFrameFactory {
 public:
  using InitCb = base::Callback<void(scoped_refptr<SurfaceTextureGLOwner>)>;

  explicit VideoFrameFactory(
      scoped_refptr<base::SingleThreadTaskRunner> gpu_task_runner);
  ~VideoFrameFactory();

  void Initialize(
      base::Callback<gpu::GpuCommandBufferStub*()> get_command_buffer_stub_cb,
      InitCb init_cb);

  void NewVideoFrame(
      MediaCodecWrapper* codec,
      int output_buffer,
      base::TimeDelta timestamp,
      base::Callback<void(const scoped_refptr<VideoFrame>&)> frame_created_cb);

 private:
  scoped_refptr<base::SingleThreadTaskRunner> gpu_task_runner_;
  std::unique_ptr<VFF> vff_;

  DISALLOW_COPY_AND_ASSIGN(VideoFrameFactory);
};

// Single threaded. Gpu main thread.
// XXX: Don't know what to call this.
class VFF {
 public:
  explicit VFF(scoped_refptr<base::SingleThreadTaskRunner> gpu_task_runner);
  ~VFF();

  scoped_refptr<SurfaceTextureGLOwner> Initialize(
      base::Callback<gpu::GpuCommandBufferStub*()> get_command_buffer_stub_cb);
  scoped_refptr<VideoFrame> NewVideoFrame(MediaCodecWrapper* codec,
                                          int output_buffer,
                                          base::TimeDelta timestamp);

 private:
  scoped_refptr<base::SingleThreadTaskRunner> gpu_task_runner_;

  // XXX: Fix the name.
  scoped_refptr<base::SingleThreadTaskRunner> client_task_runner_;
  base::WeakPtr<gpu::gles2::GLES2Decoder> gl_decoder_;

  DISALLOW_COPY_AND_ASSIGN(VFF);
};

}  // namespace media

#endif  // MEDIA_GPU_ANDROID_VIDEO_FRAME_FACTORY_
