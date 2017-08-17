// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_GPU_GPU_ARC_VIDEO_DECODE_ACCELERATOR_H_
#define CHROME_GPU_GPU_ARC_VIDEO_DECODE_ACCELERATOR_H_

#include <vector>

#include "base/threading/thread_checker.h"
#include "chrome/gpu/arc_video_decode_accelerator.h"
#include "components/arc/common/video_decode_accelerator.mojom.h"
#include "gpu/command_buffer/service/gpu_preferences.h"

namespace chromeos {
namespace arc {
class GpuArcVideoDecodeAccelerator
    : public ::arc::mojom::VideoDecodeAccelerator,
      public ArcVideoDecodeAccelerator::Client {
 public:
  explicit GpuArcVideoDecodeAccelerator(
      const gpu::GpuPreferences& gpu_preferences);
  ~GpuArcVideoDecodeAccelerator() override;

 private:
  // ArcVideoDecodeAccelerator::Client implementation.
  void ProvidePictureBuffers(uint32_t requested_num_of_buffers,
                             media::VideoPixelFormat format,
                             uint32_t textures_per_buffer,
                             const gfx::Size& dimensions,
                             uint32_t texture_target) override;
  void PictureReady(const media::Picture& picture) override;
  void NotifyEndOfBitstreamBuffer(int32_t bitstream_buffer) override;
  void NotifyFlushDone() override;
  void NotifyResetDone() override;
  void NotifyError(ArcVideoDecodeAccelerator::Result error) override;

  // ::arc::mojom::VideoDecodeAccelerator implementation.
  void Initialize(media::VideoCodecProfile profile,
                  ::arc::mojom::VideoDecodeClientPtr client,
                  const InitializeCallback& callback);
  void Decode(::arc::mojom::BitstreamBufferPtr bitstream_buffer);
  void AssignPictureBuffers(uint32_t count);
  void ImportBufferForPicture(int32_t picture_id,
                              mojo::ScopedHandle dmabuf_handle,
                              std::vector<::arc::VideoFramePlane> planes);
  void ReusePictureBuffer(int32_t picture_id);
  void Flush();
  void Reset();

  base::ScopedFD UnwrapFdFromMojoHandle(mojo::ScopedHandle handle);


  gpu::GpuPreferences gpu_preferences_;
  std::unique_ptr<ArcVideoDecodeAccelerator> accelerator_;
  ::arc::mojom::VideoDecodeClientPtr client_;

  THREAD_CHECKER(thread_checker_);
  DISALLOW_COPY_AND_ASSIGN(GpuArcVideoDecodeAccelerator);
};

}  // namespace arc
}  // namespace chromeos

#endif  // CHROME_GPU_GPU_ARC_VIDEO_DECODE_ACCELERATOR_H_
