// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/gpu/vaapi_jpeg_encode_accelerator.h"

#include <stddef.h>
#include <string.h>

#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/logging.h"
#include "base/metrics/histogram_macros.h"
#include "base/task_scheduler/post_task.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/trace_event/trace_event.h"
#include "gpu/ipc/service/gpu_channel.h"
#include "media/base/video_frame.h"
#include "media/filters/jpeg_parser.h"
#include "media/gpu/shared_memory_region.h"
#include "media/gpu/vaapi_jpeg_encoder.h"
#include "media/gpu/vaapi_picture.h"

namespace media {

namespace {

// UMA results that the VaapiJpegEncodeAccelerator class reports.
// These values are persisted to logs, and should therefore never be renumbered
// nor reused.
enum VAJEAEncoderResult {
  VAAPI_SUCCESS = 0,
  VAAPI_ERROR,
  VAJEA_ENCODER_RESULT_MAX = VAAPI_ERROR,
};

static void ReportToUMA(VAJEAEncoderResult result) {
  UMA_HISTOGRAM_ENUMERATION("Media.VAJEA.EncoderResult", result,
                            VAJEAEncoderResult::VAJEA_ENCODER_RESULT_MAX + 1);
}
}  // namespace

VaapiJpegEncodeAccelerator::EncodeRequest::EncodeRequest(
    scoped_refptr<media::VideoFrame> video_frame,
    BitstreamBuffer* output_buffer,
    int quality)
    : video_frame(video_frame),
      output_buffer(output_buffer),
      quality(quality) {}

VaapiJpegEncodeAccelerator::EncodeRequest::~EncodeRequest() {}

VaapiJpegEncodeAccelerator::Encoder::Encoder(
    scoped_refptr<VaapiWrapper> vaapi_wrapper,
    scoped_refptr<base::SingleThreadTaskRunner> task_runner,
    base::WeakPtr<VaapiJpegEncodeAccelerator> weak_this)
    : cached_output_buffer_size_(0),
      vaapi_wrapper_(vaapi_wrapper),
      task_runner_(task_runner),
      weak_this_(weak_this) {}

VaapiJpegEncodeAccelerator::Encoder::~Encoder() {}

void VaapiJpegEncodeAccelerator::Encoder::NotifyErrorFromEncoderThread(
    int video_frame_id,
    Status status) {
  task_runner_->PostTask(FROM_HERE,
                         base::Bind(&VaapiJpegEncodeAccelerator::NotifyError,
                                    weak_this_, video_frame_id, status));
}

void VaapiJpegEncodeAccelerator::Encoder::EncodeTask(
    std::unique_ptr<EncodeRequest> request) {
  TRACE_EVENT0("jpeg", "EncodeTask");

  // SharedMemoryRegion will take over the |bitstream_buffer.handle()|.
  std::unique_ptr<SharedMemoryRegion> shm(
      new SharedMemoryRegion(*request->output_buffer, false));
  const int video_frame_id = request->video_frame->unique_id();
  if (!shm->Map()) {
    LOG(ERROR) << "Failed to map output buffer";
    NotifyErrorFromEncoderThread(video_frame_id, INACCESSIBLE_OUTPUT_BUFFER);
    return;
  }

  gfx::Size coded_size = request->video_frame->coded_size();
  std::vector<VASurfaceID> va_surfaces;
  if (!vaapi_wrapper_->CreateSurfaces(VA_RT_FORMAT_YUV420, coded_size, 1,
                                      &va_surfaces) ||
      va_surfaces.empty()) {
    LOG(ERROR) << "Failed to create VA surface";
    NotifyErrorFromEncoderThread(video_frame_id, PLATFORM_FAILURE);
    return;
  }
  VASurfaceID va_surface_id = va_surfaces[0];

  if (!vaapi_wrapper_->UploadVideoFrameToSurface(request->video_frame,
                                                 va_surface_id)) {
    LOG(ERROR) << "Failed to upload video frame to VA surface";
    NotifyErrorFromEncoderThread(video_frame_id, PLATFORM_FAILURE);
    return;
  }

  // Create output buffer for encoding result.
  gfx::Size input_size = request->video_frame->coded_size();
  size_t max_coded_buffer = vaapi_jpeg_encoder::GetMaxCodedBufferSize(
      input_size.width(), input_size.height());
  if (max_coded_buffer > cached_output_buffer_size_) {
    vaapi_wrapper_->DestroyCodedBuffers();
    cached_output_buffer_size_ = 0;

    VABufferID output_buffer_id;
    if (!vaapi_wrapper_->CreateCodedBuffer(max_coded_buffer,
                                           &output_buffer_id)) {
      LOG(ERROR) << "Failed to create VA buffer for encoding output";
      NotifyErrorFromEncoderThread(video_frame_id, PLATFORM_FAILURE);
      return;
    }
    cached_output_buffer_size_ = max_coded_buffer;
    cached_output_buffer_id_ = output_buffer_id;
  }

  if (!vaapi_jpeg_encoder::Encode(
          vaapi_wrapper_.get(), request->video_frame->coded_size(),
          request->quality, va_surface_id, cached_output_buffer_id_)) {
    LOG(ERROR) << "Encode JPEG failed";
    NotifyErrorFromEncoderThread(video_frame_id, PLATFORM_FAILURE);
    return;
  }

  // Get the encoded output.
  size_t encoded_size = 0;
  if (!vaapi_wrapper_->DownloadFromCodedBuffer(
          cached_output_buffer_id_, va_surface_id,
          static_cast<uint8_t*>(shm->memory()), shm->size(), &encoded_size)) {
    LOG(ERROR) << "Failed to retrieve output image from VA coded buffer";
    NotifyErrorFromEncoderThread(video_frame_id, PLATFORM_FAILURE);
  }

  task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&VaapiJpegEncodeAccelerator::VideoFrameReady, weak_this_,
                 request->video_frame->unique_id(), encoded_size));
}

VaapiJpegEncodeAccelerator::VaapiJpegEncodeAccelerator(
    scoped_refptr<base::SingleThreadTaskRunner> io_task_runner)
    : task_runner_(base::ThreadTaskRunnerHandle::Get()),
      io_task_runner_(io_task_runner),
      weak_this_factory_(this) {}

VaapiJpegEncodeAccelerator::~VaapiJpegEncodeAccelerator() {
  DCHECK(task_runner_->BelongsToCurrentThread());
  DVLOG(1) << "Destroying VaapiJpegEncodeAccelerator";

  weak_this_factory_.InvalidateWeakPtrs();
  encoder_task_runner_->DeleteSoon(FROM_HERE, std::move(encoder_));
}

void VaapiJpegEncodeAccelerator::NotifyError(int video_frame_id,
                                             Status status) {
  DCHECK(task_runner_->BelongsToCurrentThread());
  DLOG(ERROR) << "Notifying error: " << status;
  DCHECK(client_);
  client_->NotifyError(video_frame_id, status);
}

void VaapiJpegEncodeAccelerator::VideoFrameReady(int video_frame_id,
                                                 size_t encoded_picture_size) {
  DCHECK(task_runner_->BelongsToCurrentThread());
  ReportToUMA(VAJEAEncoderResult::VAAPI_SUCCESS);

  client_->VideoFrameReady(video_frame_id, encoded_picture_size);
}

JpegEncodeAccelerator::Status VaapiJpegEncodeAccelerator::Initialize(
    JpegEncodeAccelerator::Client* client) {
  DCHECK(task_runner_->BelongsToCurrentThread());

  if (!VaapiWrapper::IsJpegEncodeSupported()) {
    return HW_JPEG_ENCODE_NOT_SUPPORTED;
  }

  client_ = client;
  scoped_refptr<VaapiWrapper> vaapi_wrapper = VaapiWrapper::Create(
      VaapiWrapper::kEncodeJpeg, VAProfileJPEGBaseline,
      base::Bind(&ReportToUMA, VAJEAEncoderResult::VAAPI_ERROR));

  if (!vaapi_wrapper) {
    LOG(ERROR) << "Failed initializing VAAPI";
    return PLATFORM_FAILURE;
  }

  encoder_task_runner_ = base::CreateSingleThreadTaskRunnerWithTraits(
      {base::MayBlock(), base::TaskPriority::USER_BLOCKING});
  if (!encoder_task_runner_) {
    LOG(ERROR) << "Failed to create encoder task runner.";
    return THREAD_CREATION_FAILED;
  }

  encoder_.reset(new Encoder(vaapi_wrapper, task_runner_,
                             weak_this_factory_.GetWeakPtr()));

  return ENCODE_OK;
}

size_t VaapiJpegEncodeAccelerator::GetMaxCodedBufferSize(
    const gfx::Size& picture_size) {
  return vaapi_jpeg_encoder::GetMaxCodedBufferSize(picture_size.width(),
                                                   picture_size.height());
}

void VaapiJpegEncodeAccelerator::Encode(
    scoped_refptr<media::VideoFrame> video_frame,
    int quality,
    BitstreamBuffer* bitstream_buffer) {
  DCHECK(io_task_runner_->BelongsToCurrentThread());

  int video_frame_id = video_frame->unique_id();
  TRACE_EVENT1("jpeg", "Encode", "input_id", video_frame_id);

  // TODO(shenghao): support other YUV formats.
  if (video_frame->format() != VideoPixelFormat::PIXEL_FORMAT_I420) {
    LOG(ERROR) << "Unsupported input format: " << video_frame->format();
    encoder_->NotifyErrorFromEncoderThread(video_frame_id, INVALID_ARGUMENT);
    return;
  }

  std::unique_ptr<EncodeRequest> request(
      new EncodeRequest(video_frame, bitstream_buffer, quality));

  encoder_task_runner_->PostTask(
      FROM_HERE, base::Bind(&VaapiJpegEncodeAccelerator::Encoder::EncodeTask,
                            base::Passed(&encoder_), base::Passed(&request)));
}

}  // namespace media
