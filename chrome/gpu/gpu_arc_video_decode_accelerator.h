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
  // The results of Decode, Flush and Reset are reported through
  // callbacks which are passed from client as their argument.
  // Each callback for Decode, Flush and Reset should be called in
  // NotifyEndofBitstreamBuffer, NotifyFlushDone and NotifyResetDone,
  // respectively.
  // They should be processed in FIFO order. That is, for example,
  // if GAVDA::Decode is called first and then GAVDA::Flush is called,
  // VDA::Flush is pending until VDA::Decode is processed and the callback
  // is called in GAVDA::NotifyEndofBitstreamBuffer.
  // CallbackManager manages callbacks to be processed as mentioned above.
  // It also checks the callbacks are called in FIFO order.
  // If it detects the order isn't FIFO, NotifyError is called.
  class CallbackManager {
   public:
    enum CallbackType : uint32_t {
      DECODE,
      RESET,
      FLUSH,
    };
    struct VDACallback {
      DecodeCallback decode_callback;
      ResetCallback reset_callback;
      FlushCallback flush_callback;
    };
    // Pushing the necessary info of callbacks to callback_infos.
    // |callback| is a given callback from client.
    // |func| is a vda function which should be called when the info
    // is top of the queue.
    // |bitstream_id| is needed in RegisterDecodeCallback in order to
    // check if the order of NotifyEndofBitstreamBuffer() is the same as
    // the order of Decode().
    void RegisterDecodeCallback(const DecodeCallback& callback,
                                base::Closure func,
                                int32_t bitstream_id);
    void RegisterResetCallback(const ResetCallback& callback,
                               base::Closure func);
    void RegisterFlushCallback(const FlushCallback& callback,
                               base::Closure func);

    // Check the order of processed callback is correct.
    bool IsValidCallback(CallbackType type, int32_t id);

    // Return the callback in top of callback_infos.
    VDACallback PopTopCallback();

    // Execute the vda function in top of callback_infos.
    // This should be called when a node is a top of callback_infos.
    void ExecuteNextFunc();

   private:
    struct CallbackInfo {
      CallbackType type;
      VDACallback callback;
      base::Closure func;
      bool is_processing = false;
      int32_t bitstream_id = -1;
      CallbackInfo(CallbackType t, VDACallback c, base::Closure f, int id = -1)
          : type(t),
            callback(std::move(c)),
            func(std::move(f)),
            bitstream_id(id) {}
    };
    std::queue<CallbackInfo> callback_infos;
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
  void Decode(::arc::mojom::BitstreamBufferPtr bitstream_buffer,
              const DecodeCallback& callback) override;
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
