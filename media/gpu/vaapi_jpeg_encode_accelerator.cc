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

// UMA errors that the VaapiJpegEncodeAccelerator class reports.
enum VAJEAEncoderFailure {
  VAAPI_ERROR = 0,
  VAJEA_ENCODER_FAILURES_MAX,
};

static void ReportToUMA(VAJEAEncoderFailure failure) {
  UMA_HISTOGRAM_ENUMERATION("Media.VAJEA.EncoderFailure", failure,
                            VAJEA_ENCODER_FAILURES_MAX + 1);
}
}  // namespace

VaapiJpegEncodeAccelerator::EncodeRequest::EncodeRequest(
    const scoped_refptr<media::VideoFrame>& video_frame,
    std::unique_ptr<SharedMemoryRegion> shm,
    int quality)
    : video_frame(video_frame), shm(std::move(shm)), quality(50) {}

VaapiJpegEncodeAccelerator::EncodeRequest::~EncodeRequest() {}

VaapiJpegEncodeAccelerator::VaapiJpegEncodeAccelerator(
    const scoped_refptr<base::SingleThreadTaskRunner>& io_task_runner)
    : task_runner_(base::ThreadTaskRunnerHandle::Get()),
      io_task_runner_(io_task_runner),
      encoder_thread_("VaapiJpegEncoderThread"),
      weak_this_factory_(this) {
  weak_this_ = weak_this_factory_.GetWeakPtr();
}

VaapiJpegEncodeAccelerator::~VaapiJpegEncodeAccelerator() {
  DCHECK(task_runner_->BelongsToCurrentThread());
  DVLOG(1) << "Destroying VaapiJpegEncodeAccelerator";

  weak_this_factory_.InvalidateWeakPtrs();
  encoder_thread_.Stop();
}

void VaapiJpegEncodeAccelerator::NotifyError(int video_frame_id, Error error) {
  DCHECK(task_runner_->BelongsToCurrentThread());
  DLOG(ERROR) << "Notifying error: " << error;
  DCHECK(client_);
  client_->NotifyError(video_frame_id, error);
}

void VaapiJpegEncodeAccelerator::NotifyErrorFromEncoderThread(
    int video_frame_id,
    Error error) {
  DCHECK(encoder_task_runner_->BelongsToCurrentThread());
  task_runner_->PostTask(FROM_HERE,
                         base::Bind(&VaapiJpegEncodeAccelerator::NotifyError,
                                    weak_this_, video_frame_id, error));
}

void VaapiJpegEncodeAccelerator::VideoFrameReady(int video_frame_id,
                                                 size_t encoded_picture_size) {
  DCHECK(task_runner_->BelongsToCurrentThread());

  base::TimeDelta elapsed = base::TimeTicks::Now() - start_encode_timestamp_;
  DVLOG(3) << "Hardware encode elapsed microseconds: "
           << elapsed.InMicroseconds();

  client_->VideoFrameReady(video_frame_id, encoded_picture_size);
}

void VaapiJpegEncodeAccelerator::EncodeTask(
    const std::unique_ptr<EncodeRequest>& request) {
  DVLOG(3) << __func__;
  DCHECK(encoder_task_runner_->BelongsToCurrentThread());
  TRACE_EVENT0("jpeg", "EncodeTask");

  const int video_frame_id = request->video_frame->unique_id();
  gfx::Size coded_size = request->video_frame->coded_size();
  std::vector<VASurfaceID> va_surfaces;
  if (!vaapi_wrapper_->CreateSurfaces(VA_RT_FORMAT_YUV420, coded_size, 1,
                                      &va_surfaces)) {
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
  size_t max_coded_buffer = VaapiJpegEncoder::GetMaxCodedBufferSize(
      input_size.width(), input_size.height());
  VABufferID output_buffer_id;
  if (!vaapi_wrapper_->CreateCodedBuffer(max_coded_buffer, &output_buffer_id)) {
    LOG(ERROR) << "Failed to create VA buffer for encoding output";
    NotifyErrorFromEncoderThread(video_frame_id, PLATFORM_FAILURE);
    return;
  }

  if (!VaapiJpegEncoder::Encode(vaapi_wrapper_.get(), request->video_frame,
                                request->quality, va_surface_id,
                                output_buffer_id)) {
    LOG(ERROR) << "Encode JPEG failed";
    NotifyErrorFromEncoderThread(video_frame_id, PLATFORM_FAILURE);
    return;
  }

  // Get the encoded output
  size_t encoded_size = 0;
  if (!vaapi_wrapper_->DownloadAndDestroyCodedBuffer(
          output_buffer_id, va_surface_id,
          static_cast<uint8_t*>(request->shm->memory()), request->shm->size(),
          &encoded_size)) {
    LOG(ERROR) << "Failed to retrieve output image from VA coded buffer";
    NotifyErrorFromEncoderThread(video_frame_id, PLATFORM_FAILURE);
  }

  task_runner_->PostTask(
      FROM_HERE, base::Bind(&VaapiJpegEncodeAccelerator::VideoFrameReady,
                            weak_this_, video_frame_id, encoded_size));
}

bool VaapiJpegEncodeAccelerator::Initialize(
    JpegEncodeAccelerator::Client* client) {
  DCHECK(task_runner_->BelongsToCurrentThread());

  client_ = client;

  vaapi_wrapper_ =
      VaapiWrapper::Create(VaapiWrapper::kEncodeJpeg, VAProfileJPEGBaseline,
                           base::Bind(&ReportToUMA, VAAPI_ERROR));

  if (!vaapi_wrapper_.get()) {
    LOG(ERROR) << "Failed initializing VAAPI";
    return false;
  }

  if (!encoder_thread_.Start()) {
    LOG(ERROR) << "Failed to start encoder thread.";
    return false;
  }
  encoder_task_runner_ = encoder_thread_.task_runner();

  return true;
}

size_t VaapiJpegEncodeAccelerator::GetMaxCodedBufferSize(int width,
                                                         int height) {
  return VaapiJpegEncoder::GetMaxCodedBufferSize(width, height);
}

void VaapiJpegEncodeAccelerator::Encode(
    const scoped_refptr<media::VideoFrame>& video_frame,
    int quality,
    BitstreamBuffer* bitstream_buffer) {
  DVLOG(3) << __func__;
  DCHECK(io_task_runner_->BelongsToCurrentThread());

  int video_frame_id = video_frame->unique_id();
  TRACE_EVENT1("jpeg", "Encode", "input_id", video_frame_id);

  start_encode_timestamp_ = base::TimeTicks::Now();

  // TODO(shenghao): support other YUV formats.
  if (video_frame->format() != VideoPixelFormat::PIXEL_FORMAT_I420) {
    LOG(ERROR) << "Unsupported input format: " << video_frame->format();
    NotifyErrorFromEncoderThread(video_frame_id, INVALID_ARGUMENT);
    return;
  }

  // SharedMemoryRegion will take over the |bitstream_buffer.handle()|.
  std::unique_ptr<SharedMemoryRegion> shm(
      new SharedMemoryRegion(*bitstream_buffer, false));

  if (!shm->Map()) {
    LOG(ERROR) << "Failed to map output buffer";
    NotifyErrorFromEncoderThread(video_frame_id, INACCESSIBLE_OUTPUT_BUFFER);
    return;
  }

  std::unique_ptr<EncodeRequest> request(
      new EncodeRequest(video_frame, std::move(shm), quality));

  encoder_task_runner_->PostTask(
      FROM_HERE, base::Bind(&VaapiJpegEncodeAccelerator::EncodeTask,
                            base::Unretained(this), base::Passed(&request)));
}

bool VaapiJpegEncodeAccelerator::IsSupported() {
  return VaapiWrapper::IsJpegEncodeSupported();
  ;
}

}  // namespace media
