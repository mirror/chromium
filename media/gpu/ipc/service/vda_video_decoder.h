// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_GPU_IPC_SERVICE_VDA_VIDEO_DECODER_H_
#define MEDIA_GPU_IPC_SERVICE_VDA_VIDEO_DECODER_H_

#include "base/callback_forward.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/single_thread_task_runner.h"
#include "media/base/video_decoder.h"
#include "media/video/video_decode_accelerator.h"

namespace gpu {
class CommandBufferStub;
}  // namespace gpu

namespace media {

// Implements the VideoDecoder interface backed by a VideoDecodeAccelerator.
// This class expects to run in the GPU process via MojoVideoDecoder.
class VdaVideoDecoder : public VideoDecoder,
                        public VideoDecodeAccelerator::Client {
 public:
  using GetStubCB = base::RepeatingCallback<gpu::CommandBufferStub*()>;

  // |task_runner|: Task runner that |this| should operate on. All methods must
  //     be called on |task_runner|.
  // |gpu_task_runner|: Task runner that |get_stub_cb| and GPU command buffer
  //     methods must be called on (the GPU main thread).
  // |get_stub_cb|: Callback to retrieve the CommandBufferStub that should be
  //     used for allocating textures and mailboxes.
  VdaVideoDecoder(scoped_refptr<base::SingleThreadTaskRunner> task_runner,
                  scoped_refptr<base::SingleThreadTaskRunner> gpu_task_runner,
                  GetStubCB get_stub_cb);

  // media::VideoDecoder implementation.
  std::string GetDisplayName() const override;
  void Initialize(const VideoDecoderConfig& config,
                  bool low_delay,
                  CdmContext* cdm_context,
                  const InitCB& init_cb,
                  const OutputCB& output_cb) override;
  void Decode(const scoped_refptr<DecoderBuffer>& buffer,
              const DecodeCB& decode_cb) override;
  void Reset(const base::RepeatingClosure& reset_cb) override;
  bool NeedsBitstreamConversion() const override;
  bool CanReadWithoutStalling() const override;
  int GetMaxDecodeRequests() const override;

 private:
  // Schedule the destriction of |this|. Destruction is asynchronous, since
  // hopping through the GPU thread may be required (and VideoDecodeAccelerator
  // destruction is similarly asynchronous).
  void Destroy() override;

  // Owners should hold VdaVideoDecoders via a std::unique_ptr, which will call
  // Destroy() automatically.
  ~VdaVideoDecoder() override;

  // media::VideoDecodeAccelerator::Client implementation.
  void NotifyInitializationComplete(bool success) override;
  void ProvidePictureBuffers(uint32_t requested_num_of_buffers,
                             VideoPixelFormat format,
                             uint32_t textures_per_buffer,
                             const gfx::Size& dimensions,
                             uint32_t texture_target) override;
  void DismissPictureBuffer(int32_t picture_buffer_id) override;
  void PictureReady(const Picture& picture) override;
  void NotifyEndOfBitstreamBuffer(int32_t bitstream_buffer_id) override;
  void NotifyFlushDone() override;
  void NotifyResetDone() override;
  void NotifyError(VideoDecodeAccelerator::Error error) override;

  DISALLOW_COPY_AND_ASSIGN(VdaVideoDecoder);
};

}  // namespace media

namespace std {

// Specialize std::default_delete to call Destroy().
// TODO(sandersd): EXPORT macro?
template <>
struct default_delete<media::VdaVideoDecoder>
    : public default_delete<media::VideoDecoder> {};

}  // namespace std

#endif  // MEDIA_GPU_IPC_SERVICE_VDA_VIDEO_DECODER_H_
