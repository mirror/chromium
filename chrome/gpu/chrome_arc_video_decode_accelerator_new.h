// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_GPU_CHROME_ARC_VIDEO_DECODE_ACCELERATOR_NEW_H_
#define CHROME_GPU_CHROME_ARC_VIDEO_DECODE_ACCELERATOR_NEW_H_

#include "base/callback.h"
#include "base/threading/thread_checker.h"
#include "chrome/gpu/arc_video_decode_accelerator_new.h"
#include "gpu/command_buffer/service/gpu_preferences.h"
#include "media/video/video_decode_accelerator.h"

namespace chromeos {
namespace arc {

// This class is executed in the GPU process. It takes decoding requests from
// ARC via IPC channels and translates and sends those requests to an
// implementation of media::VideoDecodeAccelerator. It also returns the decoded
// frames back to the ARC side.
class ChromeArcVideoDecodeAcceleratorNew
    : public ArcVideoDecodeAcceleratorNew,
      public media::VideoDecodeAccelerator::Client,
      public base::SupportsWeakPtr<ChromeArcVideoDecodeAcceleratorNew> {
 public:
  explicit ChromeArcVideoDecodeAcceleratorNew(
      const gpu::GpuPreferences& gpu_preferences);
  ~ChromeArcVideoDecodeAcceleratorNew() override;

  // Implementation of the ArcVideoDecodeAcceleratorNew interface.
  ArcVideoDecodeAcceleratorNew::Result Initialize(
      media::VideoCodecProfile profile,
      ArcVideoDecodeAcceleratorNew::Client* client) override;
  void Decode(BitstreamBuffer&& bitstream_buffer) override;
  void AssignPictureBuffers(uint32_t count) override;
  void ImportBufferForPicture(
      int32_t picture_id,
      base::ScopedFD ashmem_fd,
      std::vector<::arc::VideoFramePlane> planes) override;
  void ReusePictureBuffer(int32_t picture_id) override;
  void Reset() override;
  void Flush() override;

  // Implementation of the VideoDecodeAccelerator::Client interface.
  void ProvidePictureBuffers(uint32_t requested_num_of_buffers,
                             media::VideoPixelFormat output_format,
                             uint32_t textures_per_buffer,
                             const gfx::Size& dimensions,
                             uint32_t texture_target) override;
  void DismissPictureBuffer(int32_t picture_buffer) override;
  void PictureReady(const media::Picture& picture) override;
  void NotifyEndOfBitstreamBuffer(int32_t bitstream_buffer_id) override;
  void NotifyFlushDone() override;
  void NotifyResetDone() override;
  void NotifyError(media::VideoDecodeAccelerator::Error error) override;

 private:
  bool VerifyDmabuf(const base::ScopedFD& dmabuf_fd,
                    const std::vector<::arc::VideoFramePlane>& planes) const;

  // Global counter that keeps track the number of active clients (i.e., how
  // many VDAs in use by this class).
  // Since this class only works on the same thread, it's safe to access
  // |client_count_| without lock.
  static int client_count_;

  ArcVideoDecodeAcceleratorNew::Client* arc_client_;

  media::VideoPixelFormat output_pixel_format_;

  gpu::GpuPreferences gpu_preferences_;

  std::unique_ptr<media::VideoDecodeAccelerator> vda_;

  gfx::Size coded_size_;

  THREAD_CHECKER(thread_checker_);
  DISALLOW_COPY_AND_ASSIGN(ChromeArcVideoDecodeAcceleratorNew);
};
}  // namespace arc
}  // namespace chromeos

#endif  // CHROME_GPU_CHROME_ARC_VIDEO_DECODE_ACCELERATOR_NEW_H_
