// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_GPU_GPU_ARC_VIDEO_DECODE_ACCELERATOR_NEW_H_
#define CHROME_GPU_GPU_ARC_VIDEO_DECODE_ACCELERATOR_NEW_H_

#include <vector>

#include "base/files/scoped_file.h"
#include "base/macros.h"
#include "base/threading/thread_checker.h"
#include "chrome/gpu/arc_video_decode_accelerator_new.h"
#include "components/arc/common/video_decode_accelerator_new.mojom.h"
#include "components/arc/video_accelerator/video_accelerator.h"
#include "gpu/command_buffer/service/gpu_preferences.h"

namespace chromeos {
namespace arc {
class GpuArcVideoDecodeAcceleratorNew
    : public ::arc::mojom::VideoDecodeAcceleratorNew,
      public ArcVideoDecodeAcceleratorNew::Client {
 public:
  explicit GpuArcVideoDecodeAcceleratorNew(
      const gpu::GpuPreferences& gpu_preferences);
  ~GpuArcVideoDecodeAcceleratorNew() override;

 private:
  // VideoDecodeAcceleratorNew::Client implementation.
  void ProvidePictureBuffers(uint32_t requested_num_of_buffers,
                             media::VideoPixelFormat format,
                             uint32_t textures_per_buffer,
                             const gfx::Size& dimensions,
                             uint32_t texture_target) override;
  void PictureReady(const media::Picture& picture) override;
  void NotifyEndOfBitstreamBuffer(int32_t bitstream_buffer) override;
  void NotifyFlushDone() override;
  void NotifyResetDone() override;
  void NotifyError(ArcVideoDecodeAcceleratorNew::Result error) override;

  // ::arc::mojom::VideoDecodeAcceleratorNew implementation.
  void Initialize(media::VideoCodecProfile profile,
                  ::arc::mojom::VideoDecodeClientNewPtr client,
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

  THREAD_CHECKER(thread_checker_);
  gpu::GpuPreferences gpu_preferences_;
  std::unique_ptr<ArcVideoDecodeAcceleratorNew> accelerator_;
  ::arc::mojom::VideoDecodeClientNewPtr client_;

  DISALLOW_COPY_AND_ASSIGN(GpuArcVideoDecodeAcceleratorNew);
};

}  // namespace arc
}  // namespace chromeos

#endif  // CHROME_GPU_GPU_ARC_VIDEO_DECODE_ACCELERATOR_NEW_H_
