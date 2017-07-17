// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/capture/video/video_capture_gpu_jpeg_decoder.h"

#include <utility>

#include "base/bind.h"
#include "base/logging.h"
#include "base/metrics/histogram_macros.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/stringprintf.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/trace_event/trace_event.h"
#include "build/build_config.h"
#include "media/base/video_frame.h"
#include "media/gpu/ipc/client/gpu_jpeg_decode_accelerator_host.h"
#include "mojo/public/cpp/system/platform_handle.h"

namespace media {

VideoCaptureGpuJpegDecoder::VideoCaptureGpuJpegDecoder(
    base::OnceClosure context_ref,
    scoped_refptr<gpu::GpuChannelHost> gpu_channel_host,
    scoped_refptr<base::SingleThreadTaskRunner> io_task_runner,
    const VideoCaptureJpegDecoderFactory::ReceiveDecodedFrameCB&
        receive_decoded_frame_cb,
    scoped_refptr<base::SingleThreadTaskRunner>
        receive_decoded_frame_task_runner)
    : context_ref_(std::move(context_ref)),
      gpu_channel_host_(std::move(gpu_channel_host)),
      io_task_runner_(std::move(io_task_runner)),
      receive_decoded_frame_cb_(receive_decoded_frame_cb),
      receive_decoded_frame_task_runner_(receive_decoded_frame_task_runner),
      next_bitstream_buffer_id_(0),
      in_buffer_id_(media::JpegDecodeAccelerator::kInvalidBitstreamBufferId) {}

VideoCaptureGpuJpegDecoder::~VideoCaptureGpuJpegDecoder() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  // |decoder_| guarantees no more JpegDecodeAccelerator::Client callbacks
  // on IO thread after deletion.
  decoder_.reset();

  // |gpu_channel_host_| should outlive |decoder_|, so |gpu_channel_host_|
  // must be released after |decoder_| has been destroyed.
  gpu_channel_host_ = nullptr;
}

void VideoCaptureGpuJpegDecoder::Initialize() {
  DCHECK(CalledOnValidThread());
  return is_in_error_state_;
}

void VideoCaptureGpuJpegDecoder::DecodeCapturedData(
    const uint8_t* data,
    size_t in_buffer_size,
    const media::VideoCaptureFormat& frame_format,
    base::TimeTicks reference_time,
    base::TimeDelta timestamp,
    media::VideoCaptureDevice::Client::Buffer out_buffer) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  LazyInitializeDecoder();
  DCHECK(decoder_);

  LOG(ERROR) << "Decoding incoming data";

  TRACE_EVENT_ASYNC_BEGIN0("jpeg", "VideoCaptureGpuJpegDecoder decoding",
                           next_bitstream_buffer_id_);
  TRACE_EVENT0("jpeg", "VideoCaptureGpuJpegDecoder::DecodeCapturedData");

  // TODO(kcwu): enqueue decode requests in case decoding is not fast enough
  // (say, if decoding time is longer than 16ms for 60fps 4k image)
  if (DecodingInProgress()) {
    DVLOG(1) << "Drop captured frame. Previous jpeg frame is still decoding";
    return;
  }

  // Enlarge input buffer if necessary.
  if (!in_shared_memory_.get() ||
      in_buffer_size > in_shared_memory_->mapped_size()) {
    // Reserve 2x space to avoid frequent reallocations for initial frames.
    const size_t reserved_size = 2 * in_buffer_size;
    in_shared_memory_.reset(new base::SharedMemory);
    if (!in_shared_memory_->CreateAndMapAnonymous(reserved_size)) {
      is_in_error_state_ = true;
      LOG(WARNING) << "CreateAndMapAnonymous failed, size=" << reserved_size;
      return;
    }
  }
  memcpy(in_shared_memory_->memory(), data, in_buffer_size);

  // No need to lock for |in_buffer_id_| since IsDecoding_Locked() is false.
  in_buffer_id_ = next_bitstream_buffer_id_;
  media::BitstreamBuffer in_buffer(in_buffer_id_, in_shared_memory_->handle(),
                                   in_buffer_size);
  // Mask against 30 bits, to avoid (undefined) wraparound on signed integer.
  next_bitstream_buffer_id_ = (next_bitstream_buffer_id_ + 1) & 0x3FFFFFFF;

  // The API of |decoder_| requires us to wrap the |out_buffer| in a VideoFrame.
  const gfx::Size dimensions = frame_format.frame_size;
  std::unique_ptr<media::VideoCaptureBufferHandle> out_buffer_access =
      out_buffer.handle_provider->GetHandleForInProcessAccess();
  base::SharedMemoryHandle out_handle =
      out_buffer.handle_provider->GetNonOwnedSharedMemoryHandleForLegacyIPC();
  scoped_refptr<media::VideoFrame> out_frame =
      media::VideoFrame::WrapExternalSharedMemory(
          media::PIXEL_FORMAT_I420,          // format
          dimensions,                        // coded_size
          gfx::Rect(dimensions),             // visible_rect
          dimensions,                        // natural_size
          out_buffer_access->data(),         // data
          out_buffer_access->mapped_size(),  // data_size
          out_handle,                        // handle
          0,                                 // shared_memory_offset
          timestamp);                        // timestamp
  if (!out_frame) {
    is_in_error_state_ = true;
    LOG(ERROR) << "DecodeCapturedData: WrapExternalSharedMemory failed";
    return;
  }
  out_frame->metadata()->SetDouble(media::VideoFrameMetadata::FRAME_RATE,
                                   frame_format.frame_rate);

  out_frame->metadata()->SetTimeTicks(media::VideoFrameMetadata::REFERENCE_TIME,
                                      reference_time);

  media::mojom::VideoFrameInfoPtr out_frame_info =
      media::mojom::VideoFrameInfo::New();
  out_frame_info->timestamp = timestamp;
  out_frame_info->pixel_format = media::PIXEL_FORMAT_I420;
  out_frame_info->storage_type = media::PIXEL_STORAGE_CPU;
  out_frame_info->coded_size = dimensions;
  out_frame_info->visible_rect = gfx::Rect(dimensions);
  out_frame_info->metadata = out_frame->metadata()->CopyInternalValues();

  decode_done_closure_ = base::Bind(
      receive_decoded_frame_cb_, out_buffer.id, out_buffer.frame_feedback_id,
      base::Passed(&out_buffer.access_permission),
      base::Passed(&out_frame_info));
  decoder_->Decode(in_buffer, std::move(out_frame));
}

void VideoCaptureGpuJpegDecoder::VideoFrameReady(int32_t bitstream_buffer_id) {
  DCHECK(io_task_runner_->BelongsToCurrentThread());

  LOG(ERROR) << "Decoded frame ready";

  TRACE_EVENT0("jpeg", "VideoCaptureGpuJpegDecoder::VideoFrameReady");
  if (!has_received_decoded_frame_) {
    send_log_message_cb_.Run("Received decoded frame from Gpu Jpeg decoder");
    has_received_decoded_frame_ = true;
  }

  if (!DecodingInProgress()) {
    LOG(ERROR) << "Got decode response while not decoding";
    return;
  }

  if (bitstream_buffer_id != in_buffer_id_) {
    LOG(ERROR) << "Unexpected bitstream_buffer_id " << bitstream_buffer_id
               << ", expected " << in_buffer_id_;
    return;
  }
  in_buffer_id_ = media::JpegDecodeAccelerator::kInvalidBitstreamBufferId;

  receive_decoded_frame_task_runner_->PostTask(FROM_HERE, decode_done_closure_);
  decode_done_closure_.Reset();

  TRACE_EVENT_ASYNC_END0("jpeg", "VideoCaptureGpuJpegDecoder decoding",
                         bitstream_buffer_id);
}

void VideoCaptureGpuJpegDecoder::NotifyError(
    int32_t bitstream_buffer_id,
    media::JpegDecodeAccelerator::Error error) {
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  LOG(ERROR) << "Decode error, bitstream_buffer_id=" << bitstream_buffer_id
             << ", error=" << error;
  send_log_message_cb_.Run("Gpu Jpeg decoder failed");
  decode_done_closure_.Reset();
  is_in_error_state_ = true;
}

void VideoCaptureGpuJpegDecoder::LazyInitializeDecoder() {
  if (is_in_error_state_ || decoder_)
    return;

  TRACE_EVENT0("gpu", "VideoCaptureGpuJpegDecoder::Initialize");
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!gpu_channel_host_) {
    LOG(ERROR) << "Failed to establish GPU channel for JPEG decoder";
  } else if (gpu_channel_host_->gpu_info().jpeg_decode_accelerator_supported) {
    int32_t route_id = gpu_channel_host_->GenerateRouteID();
    auto decoder = base::MakeUnique<media::GpuJpegDecodeAcceleratorHost>(
        gpu_channel_host_.get(), route_id, io_task_runner_);
    if (decoder->Initialize(this)) {
      gpu_channel_host_->AddRouteWithTaskRunner(
          route_id, decoder->GetReceiver(), io_task_runner_);
      decoder_ = std::move(decoder);
    } else {
      DLOG(ERROR) << "Failed to initialize JPEG decoder";
    }
  }
  if (!decoder_)
    is_in_error_state_ = true;
  // TODO(chfremer): Is this duplicate with the UMA point in
  // VideoCaptureJpegDecoderFactory::IsPlatformSupported()?
  UMA_HISTOGRAM_BOOLEAN("Media.VideoCaptureGpuJpegDecoder.InitDecodeSuccess",
                        is_in_error_state_);
}

bool VideoCaptureGpuJpegDecoder::DecodingInProgress() const {
  return !decode_done_closure_.is_null();
}

}  // namespace media
