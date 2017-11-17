// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_GPU_GPU_ARC_VIDEO_DECODE_ACCELERATOR_H_
#define CHROME_GPU_GPU_ARC_VIDEO_DECODE_ACCELERATOR_H_

#include <memory>
#include <queue>
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
                "The type of PendingCallback must much ResetCallback");
  static_assert(std::is_same<FlushCallback, PendingCallback>::value,
                "The type of PendingCallback must much FlushCallback");

  // PendingRequest represents the information of a pending operation.
  // In the destructor, callback is called if it is not called yet, and
  // a handle bound by bitstream_buffer is closed if the handle is valid.
  struct PendingRequest {
    enum class Op : uint32_t {
      DECODE = 0,
      FLUSH = 1,
      RESET = 2,
    };

    PendingRequest();
    PendingRequest(PendingRequest&& pending_request) = default;
    PendingRequest(Op op, PendingCallback callback);
    ~PendingRequest();

    // |op_| denotes a currently requested operation.
    // The requested operation is  VDA::Flush(), VDA::Reset() or VDA::Decode().
    // |callback_| is null iff |op_| is Decode.
    // |bitstream_buffer_| is valid iff |op_| is Decode.
    Op op_;
    PendingCallback callback_;
    media::BitstreamBuffer bitstream_buffer_;
  };

  // Return true if |planes| is valid for |dmabuf_fd|.
  bool VerifyDmabuf(int dmabuf_fd,
                    const std::vector<::arc::VideoFramePlane>& planes) const;

  // Initialize GpuArcVDA and create VDA. It returns SUCCESS if they are
  // succeeded. Otherwise, returns an error status.
  ::arc::mojom::VideoDecodeAccelerator::Result InitializeTask(
      ::arc::mojom::VideoDecodeAcceleratorConfigPtr config);

  // Execute all pending requests until a VDA::Reset() request is encountered.
  // When that happens, we need to explicitly wait for NotifyResetDone().
  // before we continue executing subsequent requests.
  void RunPendingRequests();

  // If |pending_reset_callback_| isn't null, because GAVDA waits for finish
  // a preceding Reset(), |request| is pended by queueing in |pending_requests|.
  // If |pending_reset_callback_| is null, the requested VDA operation is
  // executed. In the case of Flush request, the callback is queued to
  // |pending_flush_callbacks_|. In the case of Reset request,
  // the callback is set |pending_reset_callback_|.
  void ExecuteRequest(PendingRequest&& request);

  // Global counter that keeps track of the number of active clients (i.e., how
  // many VDAs in use by this class).
  // Since this class only works on the same thread, it's safe to access
  // |client_count_| without lock.
  static size_t client_count_;

  // |error_state_| is true, if GAVDA gets an error from VDA.
  // All the pending functions are cancelled and the callbacks are
  // executed with an error state.
  bool error_state_ = false;

  // The variables for managing callbacks.
  // VDA::Decode(), VDA::Flush() and VDA::Reset() should not be posted to VDA
  // while the previous Reset() hasn't been finished yet (i.e. NotifyResetDone()
  // is invoked).
  // Those requests will be queued in |pending_requests_| in a FIFO manner.
  // Those pending requests will be executed once all the preceding Reset()
  // have been finished.
  // In GAVDA destructor, all the pending_requests are destructed and therefore
  // all the callbacks would be called eventually.
  // |pending_flush_callbacks_| stores all the callbacks corresponding to
  // currently executing Flush()s in VDA. |pending_reset_callback_| is a
  // callback of the currently executing Reset() in VDA.
  // If |pending_flush_callbacks_| is not empty in NotifyResetDone(), since
  // Flush()s are cancelled by Reset() in VDA, they are called with CANCELLED.
  std::queue<PendingRequest> pending_requests_;
  std::queue<FlushCallback> pending_flush_callbacks_;
  ResetCallback pending_reset_callback_;

  gpu::GpuPreferences gpu_preferences_;
  std::unique_ptr<media::VideoDecodeAccelerator> vda_;
  ::arc::mojom::VideoDecodeClientPtr client_;

  gfx::Size coded_size_;
  media::VideoPixelFormat output_pixel_format_ = media::PIXEL_FORMAT_UNKNOWN;

  // Owned by caller.
  ProtectedBufferManager* const protected_buffer_manager_;

  bool secure_mode_ = false;
  size_t output_buffer_size_ = 0;

  // Set of ProtectedBufferHandle. Even although the stored data are not used
  // for anything in GAVDA, storing them in GAVDA is needed to prevent them
  // from destructing.
  // For both of them, it is prohibited to allocate more than the determined
  // number of protected buffers.
  // The number is a constant value (kMaxProtectedInputBuffers) for protected
  // input buffers, and it is |output_buffer_size_| set in
  // AssignPictureBuffers() for protected output buffers.
  std::vector<std::unique_ptr<ProtectedBufferHandle>> protected_input_handles_;
  std::vector<std::unique_ptr<ProtectedBufferHandle>> protected_output_handles_;

  THREAD_CHECKER(thread_checker_);
  DISALLOW_COPY_AND_ASSIGN(GpuArcVideoDecodeAccelerator);
};

}  // namespace arc
}  // namespace chromeos

#endif  // CHROME_GPU_GPU_ARC_VIDEO_DECODE_ACCELERATOR_H_
