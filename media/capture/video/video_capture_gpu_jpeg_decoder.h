// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_CAPTURE_VIDEO_VIDEO_CAPTURE_GPU_JPEG_DECODER_H_
#define MEDIA_CAPTURE_VIDEO_VIDEO_CAPTURE_GPU_JPEG_DECODER_H_

#include <stddef.h>
#include <stdint.h>

#include <memory>
#include <string>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/sequence_checker.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread.h"
#include "media/capture/capture_export.h"
#include "gpu/config/gpu_info.h"
#include "media/capture/video/video_capture_jpeg_decoder.h"
#include "media/video/jpeg_decode_accelerator.h"

namespace gpu {
class GpuChannelHost;
}

namespace media {

// Adapter to GpuJpegDecodeAccelerator for VideoCaptureDevice::Client. It takes
// care of GpuJpegDecodeAccelerator creation, shared memory, and threading
// issues.
//
// All public methods except JpegDecodeAccelerator::Client ones should be called
// on the same thread. JpegDecodeAccelerator::Client methods should be called on
// the IO thread.
class CAPTURE_EXPORT VideoCaptureGpuJpegDecoder
    : public media::VideoCaptureJpegDecoder,
      public media::JpegDecodeAccelerator::Client,
      public base::SupportsWeakPtr<VideoCaptureGpuJpegDecoder> {
 public:
  explicit VideoCaptureGpuJpegDecoder(
      base::OnceClosure context_ref,
      scoped_refptr<gpu::GpuChannelHost> gpu_channel_host,
      scoped_refptr<base::SingleThreadTaskRunner>
          io_task_runner,
      const VideoCaptureJpegDecoderFactory::ReceiveDecodedFrameCB&
          receive_decoded_frame_cb,
      scoped_refptr<base::SingleThreadTaskRunner>
          receive_decoded_frame_task_runner);
  ~VideoCaptureGpuJpegDecoder() override;

  // Implementation of media::VideoCaptureJpegDecoder:
  bool IsInErrorState() const override;
  void DecodeCapturedData(
      const uint8_t* data,
      size_t in_buffer_size,
      const media::VideoCaptureFormat& frame_format,
      base::TimeTicks reference_time,
      base::TimeDelta timestamp,
      media::VideoCaptureDevice::Client::Buffer out_buffer) override;

  // JpegDecodeAccelerator::Client implementation.
  void VideoFrameReady(int32_t buffer_id) override;
  void NotifyError(int32_t buffer_id,
                   media::JpegDecodeAccelerator::Error error) override;

 private:
  static void RequestGPUInfoOnIOThread(
      scoped_refptr<base::SingleThreadTaskRunner> task_runner,
      base::WeakPtr<VideoCaptureGpuJpegDecoder> weak_this);

  static void DidReceiveGPUInfoOnIOThread(
      scoped_refptr<base::SingleThreadTaskRunner> task_runner,
      base::WeakPtr<VideoCaptureGpuJpegDecoder> weak_this,
      const gpu::GPUInfo& gpu_info);

  void LazyInitializeDecoder();
  bool DecodingInProgress() const;

  base::OnceClosure context_ref_;
  scoped_refptr<gpu::GpuChannelHost> gpu_channel_host_;
  const scoped_refptr<base::SingleThreadTaskRunner> io_task_runner_;

  // The underlying JPEG decode accelerator.
  std::unique_ptr<media::JpegDecodeAccelerator> decoder_;

  // The callback to run when decode succeeds.
  VideoCaptureJpegDecoderFactory::ReceiveDecodedFrameCB
      receive_decoded_frame_cb_;
  const scoped_refptr<base::SingleThreadTaskRunner>
      receive_decoded_frame_task_runner_;

  // The closure of |decode_done_cb_| with bound parameters.
  base::Closure decode_done_closure_;

  // Next id for input BitstreamBuffer.
  int32_t next_bitstream_buffer_id_;

  // The id for current input BitstreamBuffer being decoded.
  int32_t in_buffer_id_;

  bool is_in_error_state_ = false;

  // Shared memory to store JPEG stream buffer. The input BitstreamBuffer is
  // backed by this.
  std::unique_ptr<base::SharedMemory> in_shared_memory_;

  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(VideoCaptureGpuJpegDecoder);
};

}  // namespace media

#endif  // MEDIA_CAPTURE_VIDEO_VIDEO_CAPTURE_GPU_JPEG_DECODER_H_
