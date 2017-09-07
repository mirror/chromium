// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_GPU_GPU_ARC_VIDEO_DECODE_ACCELERATOR_H_
#define CHROME_GPU_GPU_ARC_VIDEO_DECODE_ACCELERATOR_H_

#include <map>
#include <vector>

#include "base/threading/thread_checker.h"
#include "components/arc/common/video_decode_accelerator.mojom.h"
#include "gpu/command_buffer/service/gpu_preferences.h"
#include "media/video/video_decode_accelerator.h"

namespace chromeos {
namespace arc {

class GpuArcVideoDecodeAccelerator
    : public ::arc::mojom::VideoDecodeAccelerator,
      public media::VideoDecodeAccelerator::Client {
 public:
  explicit GpuArcVideoDecodeAccelerator(
      const gpu::GpuPreferences& gpu_preferences);
  ~GpuArcVideoDecodeAccelerator() override;

 private:
  // Implementation of media::VideoDecodeAccelerator::Client interface.
  void ProvidePictureBuffers(uint32_t requested_num_of_buffers,
                             media::VideoPixelFormat format,
                             uint32_t textures_per_buffer,
                             const gfx::Size& dimensions,
                             uint32_t texture_target) override;
  void PictureReady(const media::Picture& picture) override;
  void DismissPictureBuffer(int32_t picture_buffer_id) override;
  void NotifyEndOfBitstreamBuffer(int32_t bitstream_buffer_id) override;
  void NotifyFlushDone() override;
  void NotifyResetDone() override;
  void NotifyError(media::VideoDecodeAccelerator::Error error) override;

  // ::arc::mojom::VideoDecodeAccelerator implementation.
  void Initialize(media::VideoCodecProfile profile,
                  ::arc::mojom::VideoDecodeClientPtr client,
                  const InitializeCallback& callback) override;
  void Decode(::arc::mojom::BitstreamBufferPtr bitstream_buffer) override;
  void AssignPictureBuffers(uint32_t count) override;
  void ImportBufferForPicture(int32_t picture_buffer_id,
                              mojo::ScopedHandle dmabuf_handle,
                              std::vector<::arc::VideoFramePlane> planes);
  void ReusePictureBuffer(int32_t picture_buffer_id) override;
  void Flush(const FlushCallback& callback) override;
  void Reset(const ResetCallback& callback) override;

  base::ScopedFD UnwrapFdFromMojoHandle(mojo::ScopedHandle handle);

  // Return true if |planes| is valid for a dmabuf |fd|.
  bool VerifyDmabuf(const base::ScopedFD& dmabuf_fd,
                    const std::vector<::arc::VideoFramePlane>& planes) const;

  // Run the pending requests until state is changed to BUSY.
  void RunPendingRequestsUntilBusy();

  // Global counter that keeps track the number of active clients (i.e., how
  // many VDAs in use by this class).
  // Since this class only works on the same thread, it's safe to access
  // |client_count_| without lock.
  static int client_count_;

  // The variables for managing callbacks.
  // Decode(), Flush() or Reset() should not be submitted to VDA while
  // the previous Reset() or Flush() hasn't be finished yet.
  // Those calls will be queued in |pending_requests_| in a FIFO manner.
  // Those pending calls will be executed once the previous Flush()/Reset()
  // has been finished.

  // State is the current GAVDA Service status.
  // DECODING - Default state.
  // ERROR - GAVDA gets an error from VDA. All the pending functions
  //         are canceled. The callbacks are returned with an error state.
  // BUSY -  Reset() or Flush() are issued to VDA. GAVDA is waiting
  //         that NotifyResetDone() or NotifyFlushDone() are invoked.
  enum State : uint32_t {
    DECODING,
    ERROR,
    BUSY,
  };

  State state_;
  std::queue<base::Closure> pending_requests_;
  ResetCallback reset_callback_;
  FlushCallback flush_callback_;

  gpu::GpuPreferences gpu_preferences_;
  std::unique_ptr<media::VideoDecodeAccelerator> vda_;
  ::arc::mojom::VideoDecodeClientPtr client_;

  gfx::Size coded_size_;
  media::VideoPixelFormat output_pixel_format_;

  THREAD_CHECKER(thread_checker_);
  DISALLOW_COPY_AND_ASSIGN(GpuArcVideoDecodeAccelerator);
};

}  // namespace arc
}  // namespace chromeos

#endif  // CHROME_GPU_GPU_ARC_VIDEO_DECODE_ACCELERATOR_H_
