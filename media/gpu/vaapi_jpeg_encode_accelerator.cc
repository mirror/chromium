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
#include "media/gpu/vaapi_picture.h"
#include "third_party/libyuv/include/libyuv.h"

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
}

VaapiJpegEncodeAccelerator::EncodeRequest::EncodeRequest(
    int32_t bitstream_buffer_id,
    scoped_refptr<SharedMemoryRegion> shm,
    int width, int height)
    : bitstream_buffer_id(bitstream_buffer_id),
      shm(shm),
      width(width),
      height(height) {}

VaapiJpegEncodeAccelerator::EncodeRequest::~EncodeRequest() {}

VaapiJpegEncodeAccelerator::VaapiJpegEncodeAccelerator(
    const scoped_refptr<base::SingleThreadTaskRunner>& io_task_runner)
    : task_runner_(base::ThreadTaskRunnerHandle::Get()),
      io_task_runner_(io_task_runner),
      encoder_thread_("VaapiJpegEncoderThread"),
      va_surface_id_(VA_INVALID_SURFACE),
      weak_this_factory_(this)   {
  weak_this_ = weak_this_factory_.GetWeakPtr();
}

VaapiJpegEncodeAccelerator::~VaapiJpegEncodeAccelerator() {
  DCHECK(task_runner_->BelongsToCurrentThread());
  DVLOG(1) << "Destroying VaapiJpegEncodeAccelerator";

  weak_this_factory_.InvalidateWeakPtrs();
  encoder_thread_.Stop();
}

void VaapiJpegEncodeAccelerator::NotifyError(int32_t bitstream_buffer_id,
                                             Error error) {
  DCHECK(task_runner_->BelongsToCurrentThread());
  DLOG(ERROR) << "Notifying of error " << error;
  DCHECK(client_);
  client_->NotifyError(bitstream_buffer_id, error);
}

void VaapiJpegEncodeAccelerator::NotifyErrorFromEncoderThread(
    int32_t bitstream_buffer_id,
    Error error) {
  DCHECK(encoder_task_runner_->BelongsToCurrentThread());
  task_runner_->PostTask(FROM_HERE,
                         base::Bind(&VaapiJpegEncodeAccelerator::NotifyError,
                                    weak_this_, bitstream_buffer_id, error));
}

void VaapiJpegEncodeAccelerator::EncodeTask(
    const std::unique_ptr<EncodeRequest>& request) {
  DVLOG(3) << __func__;
  DCHECK(encoder_task_runner_->BelongsToCurrentThread());
  TRACE_EVENT0("jpeg", "EncodeTask");

  va_rt_format_ = VA_RT_FORMAT_YUV420;
  coded_size_ = gfx::Size(request->width, request->height);
  std::vector<VASurfaceID> va_surfaces;
  if (!vaapi_wrapper_->CreateSurfaces(va_rt_format_, coded_size_, 1,
                                      &va_surfaces)) {
    LOG(ERROR) << "Failed to create VA surface";
    NotifyErrorFromEncoderThread(request->bitstream_buffer_id,
                                 PLATFORM_FAILURE);
    return;
  }

  if (!vaapi_wrapper_->UploadFrameToSurface(request->shm,
      request->width, request->height, va_surface_id_)) {
    LOG(ERROR) << "Failed to upload frame content to VA surface";
    NotifyErrorFromEncoderThread(request->bitstream_buffer_id,
                                 PLATFORM_FAILURE);
    return;
  }

  if (!VaapiJpegEncoder::Encode(vaapi_wrapper_.get(),
                                request->width,
                                request->height,
                                va_surface_id_)) {
    LOG(ERROR) << "Encode JPEG failed";
    NotifyErrorFromEncoderThread(request->bitstream_buffer_id,
                                 PLATFORM_FAILURE);
    return;
  }

  // Get the encoded output

}

bool VaapiJpegEncodeAccelerator::Initialize(
    JpegEncodeAccelerator::Client* client) {
  DCHECK(task_runner_->BelongsToCurrentThread());

  client_ = client;

  vaapi_wrapper_ =
      VaapiWrapper::Create(VaapiWrapper::kEncode, VAProfileJPEGBaseline,
                           base::Bind(&ReportToUMA, VAAPI_ERROR));

  if (!vaapi_wrapper_.get()) {
    DLOG(ERROR) << "Failed initializing VAAPI";
    return false;
  }

  if (!encoder_thread_.Start()) {
    DLOG(ERROR) << "Failed to start encoder thread.";
    return false;
  }
  encoder_task_runner_ = encoder_thread_.task_runner();

  return true;
}

void VaapiJpegEncodeAccelerator::Encode(
    const BitstreamBuffer& bitstream_buffer, int width, int height) {
  DVLOG(3) << __func__;
  DCHECK(io_task_runner_->BelongsToCurrentThread());
  TRACE_EVENT1("jpeg", "Encode", "input_id", bitstream_buffer.id());

  DVLOG(4) << "Mapping new input buffer id: " << bitstream_buffer.id()
           << " size: " << bitstream_buffer.size();

  // SharedMemoryRegion will take over the |bitstream_buffer.handle()|.
  scoped_refptr<SharedMemoryRegion> shm(
      new SharedMemoryRegion(bitstream_buffer, true));

  if (bitstream_buffer.id() < 0) {
    LOG(ERROR) << "Invalid bitstream_buffer, id: " << bitstream_buffer.id();
    NotifyErrorFromEncoderThread(bitstream_buffer.id(), INVALID_ARGUMENT);
    return;
  }

  if (!shm->Map()) {
    LOG(ERROR) << "Failed to map input buffer";
    NotifyErrorFromEncoderThread(bitstream_buffer.id(), UNREADABLE_INPUT);
    return;
  }

  std::unique_ptr<EncodeRequest> request(
      new EncodeRequest(bitstream_buffer.id(), shm, width, height));

  encoder_task_runner_->PostTask(
      FROM_HERE, base::Bind(&VaapiJpegEncodeAccelerator::EncodeTask,
                            base::Unretained(this), base::Passed(&request)));
}

bool VaapiJpegEncodeAccelerator::IsSupported() {
  return VaapiWrapper::IsJpegEncodeSupported();;
}


}  // namespace media
