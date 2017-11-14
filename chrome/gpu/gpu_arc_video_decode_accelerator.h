// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_GPU_GPU_ARC_VIDEO_DECODE_ACCELERATOR_H_
#define CHROME_GPU_GPU_ARC_VIDEO_DECODE_ACCELERATOR_H_

#include <memory>
#include <queue>
#include <set>
#include <vector>

#include "base/callback_forward.h"
#include "base/threading/thread_checker.h"
#include "components/arc/common/video_decode_accelerator.mojom.h"
#include "gpu/command_buffer/service/gpu_preferences.h"
#include "media/video/video_decode_accelerator.h"

namespace chromeos {
namespace arc {

class ProtectedBufferManager;
class ProtectedBufferHandle;

// GpuArcVideoDecodeAccelerator is executed in the GPU process.
// It takes decoding requests from ARC via IPC channels and translates and
// sends those requests to an implementation of media::VideoDecodeAccelerator.
// It also calls ARC client functions in media::VideoDecodeAccelerator
// callbacks, e.g., PictureReady, which returns the decoded frames back to the
// ARC side. This class manages Reset and Flush requests and life-cycle of
// passed callback for them. They would be processed in FIFO order.

// For each creation request from GpuArcVideoDecodeAcceleratorHost,
// GpuArcVideoDecodeAccelerator will create a new IPC channel.
class GpuArcVideoDecodeAccelerator
    : public ::arc::mojom::VideoDecodeAccelerator,
      public media::VideoDecodeAccelerator::Client {
 public:
  GpuArcVideoDecodeAccelerator(
      const gpu::GpuPreferences& gpu_preferences,
      ProtectedBufferManager* protected_buffer_manager);
  ~GpuArcVideoDecodeAccelerator() override;

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
  void Initialize(::arc::mojom::VideoDecodeAcceleratorConfigPtr config,
                  ::arc::mojom::VideoDecodeClientPtr client,
                  InitializeCallback callback) override;
  void AllocateProtectedBuffer(
      mojo::ScopedHandle handle,
      uint64_t size,
      AllocateProtectedBufferCallback callback) override;
  void Decode(::arc::mojom::BitstreamBufferPtr bitstream_buffer) override;
  void AssignPictureBuffers(uint32_t count) override;
  void ImportBufferForPicture(int32_t picture_buffer_id,
                              mojo::ScopedHandle handle,
                              std::vector<::arc::VideoFramePlane> planes);
  void ReusePictureBuffer(int32_t picture_buffer_id) override;
  void Flush(FlushCallback callback) override;
  void Reset(ResetCallback callback) override;

 private:
  using PendingCallback =
      base::OnceCallback<void(::arc::mojom::VideoDecodeAccelerator::Result)>;
  static_assert(std::is_same<ResetCallback, PendingCallback>::value,
                "The type of callbacks of Reset must be pending_callback_t");
  static_assert(std::is_same<FlushCallback, PendingCallback>::value,
                "The type of callbacks of Flush must be pending_callback_t");

  // Return true if |planes| is valid for |dmabuf_fd|.
  bool VerifyDmabuf(int dmabuf_fd,
                    const std::vector<::arc::VideoFramePlane>& planes) const;

  // Initialize GpuArcVDA and create VDA. It returns SUCCESS iff they are
  // succeeded. Otherwise, returns an error status.
  ::arc::mojom::VideoDecodeAccelerator::Result InitializeTask(
      ::arc::mojom::VideoDecodeAcceleratorConfigPtr config);

  // Execute all pending requests until a request requiring a callback is
  // encountered. When that happens, we need to explicitly wait for it to
  // finish, before we continue executing subsequent requests, setting
  // current_callback_ to the callback for that request and running it after
  // the request finishes executing.
  void RunPendingRequests();

  void ExecuteRequest(base::OnceClosure&& vda_request,
                      PendingCallback&& callback);

  // Global counter that keeps track of the number of active clients (i.e., how
  // many VDAs in use by this class).
  // Since this class only works on the same thread, it's safe to access
  // |client_count_| without lock.
  static size_t client_count_;

  // The variables for managing callbacks.
  // Decode(), Flush() or Reset() should not be submitted to VDA while
  // the previous Reset() or Flush() hasn't been finished yet.
  // Those calls will be queued in |pending_requests_| in a FIFO manner.
  // Those pending calls will be executed once all the preceding Flush()/Reset()
  // have been finished.

  // |error_status_| is true, if GAVDA gets an error from VDA.
  // All the pending functions are cancelled and the callbacks are
  // executed with an error state.
  bool error_state_ = false;
  std::queue<std::pair<base::OnceClosure, PendingCallback>> pending_requests_;

  PendingCallback pending_callback_;

  gpu::GpuPreferences gpu_preferences_;
  std::unique_ptr<media::VideoDecodeAccelerator> vda_;
  ::arc::mojom::VideoDecodeClientPtr client_;

  gfx::Size coded_size_;
  media::VideoPixelFormat output_pixel_format_ = media::PIXEL_FORMAT_UNKNOWN;

  ProtectedBufferManager* const protected_buffer_manager_;
  bool secure_mode_ = false;
  uint32_t output_buffer_size_ = 0;

  // Set of ProtectedBufferHandle. Even although the stored data are not used
  // for anything in GAVDA, storing them in GAVDA is needed to prevent them
  // from destructing.
  std::set<std::unique_ptr<ProtectedBufferHandle>> protected_input_handles;
  std::set<std::unique_ptr<ProtectedBufferHandle>> protected_output_handles;
  THREAD_CHECKER(thread_checker_);
  DISALLOW_COPY_AND_ASSIGN(GpuArcVideoDecodeAccelerator);
};

}  // namespace arc
}  // namespace chromeos

#endif  // CHROME_GPU_GPU_ARC_VIDEO_DECODE_ACCELERATOR_H_
