// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_GPU_ANDROID_VIDEO_FRAME_FACTORY_IMPL_
#define MEDIA_GPU_ANDROID_VIDEO_FRAME_FACTORY_IMPL_

#include "base/optional.h"
#include "gpu/command_buffer/service/gles2_cmd_decoder.h"
#include "gpu/command_buffer/service/texture_manager.h"
#include "gpu/ipc/service/gpu_command_buffer_stub.h"
#include "media/base/video_frame.h"
#include "media/gpu/android/codec_image.h"
#include "media/gpu/android/codec_wrapper.h"
#include "media/gpu/android/surface_texture_gl_owner.h"
#include "media/gpu/android/video_frame_factory.h"
#include "media/gpu/gles2_decoder_helper.h"
#include "media/gpu/media_gpu_export.h"
#include "ui/gl/gl_bindings.h"

namespace media {
using FrameAndReleaseCb =
    std::pair<scoped_refptr<VideoFrame>, VideoFrameFactory::ReleaseMailboxCB>;

// VideoFrameFactoryImpl creates CodecOutputBuffer backed VideoFrames and tries
// to eagerly render them to their surface to release the buffers back to the
// decoder as soon as possible. It's not thread safe; it should be created, used
// and destructed on a single sequence. It's implemented by proxying calls
// to a helper class hosted on the gpu thread.
class MEDIA_GPU_EXPORT VideoFrameFactoryImpl : public VideoFrameFactory {
 public:
  // |get_stub_cb| will be run on |gpu_task_runner|.
  VideoFrameFactoryImpl(
      scoped_refptr<base::SingleThreadTaskRunner> gpu_task_runner,
      GetStubCb get_stub_cb);
  ~VideoFrameFactoryImpl() override;

  void Initialize(InitCb init_cb) override;
  void CreateVideoFrame(std::unique_ptr<CodecOutputBuffer> output_buffer,
                        scoped_refptr<SurfaceTextureGLOwner> surface_texture,
                        base::TimeDelta timestamp,
                        gfx::Size natural_size,
                        OutputWithReleaseMailboxCB output_cb) override;
  void RunAfterPendingVideoFrames(base::Closure closure) override;
  void CancelPendingCallbacks() override;

 private:
  // These member functions intercept the reponses from GpuSide and forward them
  // to the caller, but allow us to cancel all pending callbacks by invalidating
  // |weak_factory_|'s pointers.
  void OnInitializeDone(InitCb init_cb,
                        scoped_refptr<SurfaceTextureGLOwner> surface_texture);
  void OnVideoFrameCreated(OutputWithReleaseMailboxCB output_cb,
                           FrameAndReleaseCb frame_and_release_cb);
  void RunClosure(base::Closure closure);

  // The gpu thread side of the implementation.
  class GpuSide;
  std::unique_ptr<GpuSide> gpu_side_;
  scoped_refptr<base::SingleThreadTaskRunner> gpu_task_runner_;
  GetStubCb get_stub_cb_;
  base::WeakPtrFactory<VideoFrameFactoryImpl> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(VideoFrameFactoryImpl);
};

namespace internal {

// Tries to render CodecImages to their backing surfaces when it's valid to do
// so. This lets us release codec buffers back to their codecs as soon as
// possible so that decoding can progress smoothly.
// Templated on the image type for testing.
template <typename Image>
void MEDIA_GPU_EXPORT MaybeRenderEarly(std::vector<Image*>* image_vector_ptr) {
  auto& images = *image_vector_ptr;
  if (images.empty())
    return;

  // Find the latest image rendered to the front buffer (if any).
  base::Optional<size_t> front_buffer_index;
  for (int i = images.size() - 1; i >= 0; --i) {
    if (images[i]->was_rendered_to_front_buffer()) {
      front_buffer_index = i;
      break;
    }
  }

  // If there's no image in the front buffer we can safely render one.
  if (!front_buffer_index) {
    // Iterate until we successfully render one to skip over invalidated images.
    for (size_t i = 0; i < images.size(); ++i) {
      if (images[i]->RenderToFrontBuffer()) {
        front_buffer_index = i;
        break;
      }
    }
    // If we couldn't render anything there's nothing more to do.
    if (!front_buffer_index)
      return;
  }

  // Try to render the image following the front buffer to the back buffer.
  size_t back_buffer_index = *front_buffer_index + 1;
  if (back_buffer_index < images.size() &&
      images[back_buffer_index]->is_surface_texture_backed()) {
    images[back_buffer_index]->RenderToSurfaceTextureBackBuffer();
  }
}

}  // namespace internal
}  // namespace media

#endif  // MEDIA_GPU_ANDROID_VIDEO_FRAME_FACTORY_IMPL_
