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
  // The results of Flush and Reset are reported through
  // callbacks which are passed from client as their argument.
  // Each callback for Flush and Reset should be called in
  // NotifyFlushDone and NotifyResetDone, respectively.
  // They should be processed in FIFO order. That is, for example,
  // if GAVDA::Reset is called first and then GAVDA::Flush is called,
  // VDA::Flush is pending until VDA::Reset is finished and the callback
  // is called in GAVDA::NotifyResetDone.
  // CallbackManager manages callbacks to be processed as mentioned above.
  // It also checks if the callbacks are called in FIFO order.
  // If it detects the order isn't FIFO, NotifyError is called.
  // All the stored callbacks are called with an error state in
  // CallbackManager's destructor.
  // That is, it is guaranteed that all the callbacks are eventually called.
  class CallbackManager {
   public:
    using callback_t =
        base::Callback<void(::arc::mojom::VideoDecodeAccelerator::Result)>;
    static_assert(std::is_same<ResetCallback, FlushCallback>::value,
                  "ResetCallback should be the same type as FlushCallback.");
    static_assert(std::is_same<ResetCallback, callback_t>::value,
                  "ResetCallback should be the same type as callback_t.");
    enum CallbackType : uint32_t {
      RESET = 0,
      FLUSH = 1,
      DECODE = 2,
    };

    struct CallbackInfo {
      CallbackType type;
      callback_t callback;
      base::Closure func;
      bool is_processing = false;
      CallbackInfo() = default;
      CallbackInfo(CallbackType t, callback_t c, base::Closure f)
          : type(t), callback(std::move(c)), func(std::move(f)) {}
    };

    ~CallbackManager();

    // Pushing the necessary info of callbacks to callback_infos.
    // |callback| is a given callback from client.
    // |func| is a vda function which should be called when the info
    // is top of the queue.
    void RegisterCallback(CallbackType type,
                          const callback_t& callback,
                          base::Closure func);

    // Check the order of processed callback is correct.
    bool IsValidCallback(CallbackType type, int32_t id);

    // Return the top of |callback_infos|.
    CallbackInfo PopCallbackInfo();

    // Execute the vda function in top of callback_infos.
    // This should be called when a node is a top of callback_infos.
    void ExecuteNextFunc();

    // No-op function. Callback for Decode().
    static void DoNothing(::arc::mojom::VideoDecodeAccelerator::Result result) {
    }

    bool change_error_state(bool error_state) {
      return on_error_ = error_state;
    }
    bool is_error_state() const { return on_error_; }

   private:
    std::queue<CallbackInfo> callback_infos;

    // If an error happens in VDA, on_error should be true, and false otherwise.
    // Set false if creating VDA is succeeded in GAVDA::Initialize().
    // Set true if GAVDA::NotifyError is called.
    // If on_error is true, Flush and Decode are ignored.
    bool on_error_;
  };

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
  // Global counter that keeps track the number of active clients (i.e., how
  // many VDAs in use by this class).
  // Since this class only works on the same thread, it's safe to access
  // |client_count_| without lock.
  static int client_count_;

  // The variables for managing callbacks.
  CallbackManager callback_manager_;

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
