// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/gpu/gpu_arc_video_decode_accelerator.h"

#include "chrome/gpu/chrome_arc_video_decode_accelerator.h"
#include "media/base/video_frame.h"
#include "mojo/public/cpp/system/platform_handle.h"

#define VLOGF(level) VLOG(level) << __func__ << "(): "
#define DVLOGF(level) DVLOG(level) << __func__ << "(): "

// Make sure arc::mojom::VideoDecodeAccelerator::Result and
// chromeos::arc::ArcVideoDecodeAccelerator::Result match.
static_assert(
    static_cast<int>(arc::mojom::VideoDecodeAccelerator::Result::SUCCESS) ==
        chromeos::arc::ArcVideoDecodeAccelerator::SUCCESS,
    "enum mismatch");
static_assert(static_cast<int>(
                  arc::mojom::VideoDecodeAccelerator::Result::ILLEGAL_STATE) ==
                  chromeos::arc::ArcVideoDecodeAccelerator::ILLEGAL_STATE,
              "enum mismatch");
static_assert(
    static_cast<int>(
        arc::mojom::VideoDecodeAccelerator::Result::INVALID_ARGUMENT) ==
        chromeos::arc::ArcVideoDecodeAccelerator::INVALID_ARGUMENT,
    "enum mismatch");
static_assert(
    static_cast<int>(
        arc::mojom::VideoDecodeAccelerator::Result::UNREADABLE_INPUT) ==
        chromeos::arc::ArcVideoDecodeAccelerator::UNREADABLE_INPUT,
    "enum mismatch");
static_assert(
    static_cast<int>(
        arc::mojom::VideoDecodeAccelerator::Result::PLATFORM_FAILURE) ==
        chromeos::arc::ArcVideoDecodeAccelerator::PLATFORM_FAILURE,
    "enum mismatch");
static_assert(
    static_cast<int>(
        arc::mojom::VideoDecodeAccelerator::Result::INSUFFICIENT_RESOURCES) ==
        chromeos::arc::ArcVideoDecodeAccelerator::INSUFFICIENT_RESOURCES,
    "enum mismatch");

namespace mojo {

template <>
struct TypeConverter<arc::mojom::PictureBufferFormatPtr,
                     chromeos::arc::PictureBufferFormat> {
  static arc::mojom::PictureBufferFormatPtr Convert(
      const chromeos::arc::PictureBufferFormat& input) {
    arc::mojom::PictureBufferFormatPtr result =
        arc::mojom::PictureBufferFormat::New();
    result->pixel_format = input.pixel_format;
    result->buffer_size = input.buffer_size;
    result->min_num_buffers = input.min_num_buffers;
    result->coded_width = input.coded_width;
    result->coded_height = input.coded_height;
    return result;
  }
};

}  // namespace mojo

namespace chromeos {
namespace arc {

GpuArcVideoDecodeAccelerator::GpuArcVideoDecodeAccelerator(
    const gpu::GpuPreferences& gpu_preferences)
    : gpu_preferences_(gpu_preferences),
      accelerator_(new ChromeArcVideoDecodeAccelerator(gpu_preferences_)) {}

GpuArcVideoDecodeAccelerator::~GpuArcVideoDecodeAccelerator() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
}

// TODO(hiroh): This enum should be moved to arc_video_decode_accelerator_new.h.
//              This is defined here temporarily because the definition is
//              conflicted in that in arc_video_decode_accelerator.h
enum HalPixelFormatExtension {
  // The pixel formats defined in Android but are used here. They are defined
  // in "system/core/include/system/graphics.h"
  HAL_PIXEL_FORMAT_BGRA_8888 = 5,
  HAL_PIXEL_FORMAT_YCbCr_420_888 = 0x23,

  // The following formats are not defined in Android, but used in
  // ArcVideoDecodeAccelerator to identify the input format.
  HAL_PIXEL_FORMAT_H264 = 0x34363248,
  HAL_PIXEL_FORMAT_VP8 = 0x00385056,
  HAL_PIXEL_FORMAT_VP9 = 0x00395056,
};

void GpuArcVideoDecodeAccelerator::ProvidePictureBuffers(
    uint32_t requested_num_of_buffers,
    media::VideoPixelFormat format,
    uint32_t textures_per_buffer,
    const gfx::Size& dimensions,
    uint32_t texture_target) {
  DVLOGF(2);
  DCHECK(client_);

  uint32_t pixel_format;
  switch (format) {
    case media::PIXEL_FORMAT_I420:
    case media::PIXEL_FORMAT_YV12:
    case media::PIXEL_FORMAT_NV12:
    case media::PIXEL_FORMAT_NV21:
      // HAL_PIXEL_FORMAT_YCbCr_420_888 is the flexible pixel format in Android
      // which handles all 420 formats, with both orderings of chroma (CbCr and
      // CrCb) as well as planar and semi-planar layouts.
      pixel_format = HAL_PIXEL_FORMAT_YCbCr_420_888;
      break;
    case media::PIXEL_FORMAT_ARGB:
      pixel_format = HAL_PIXEL_FORMAT_BGRA_8888;
      break;
    default:
      VLOGF(1) << "Format not supported: " << format;
      NotifyError(ArcVideoDecodeAccelerator::PLATFORM_FAILURE);
      return;
  }
  chromeos::arc::PictureBufferFormat pbf;
  pbf.pixel_format = pixel_format;
  pbf.buffer_size = media::VideoFrame::AllocationSize(format, dimensions);
  pbf.min_num_buffers = requested_num_of_buffers;
  pbf.coded_width = dimensions.width();
  pbf.coded_height = dimensions.height();
  client_->ProvidePictureBuffers(::arc::mojom::PictureBufferFormat::From(pbf));
}

void GpuArcVideoDecodeAccelerator::PictureReady(const media::Picture& picture) {
  DVLOGF(2);
  DCHECK(client_);
  auto picture_buffer_id = picture.picture_buffer_id();
  auto bitstream_buffer_id = picture.bitstream_buffer_id();
  auto crop_rect = picture.visible_rect();
  client_->PictureReady(picture_buffer_id, bitstream_buffer_id, crop_rect);
}

void GpuArcVideoDecodeAccelerator::NotifyEndOfBitstreamBuffer(
    int32_t bitstream_buffer) {
  DVLOGF(2);
  DCHECK(client_);
  client_->NotifyEndOfBitstreamBuffer(bitstream_buffer);
}

void GpuArcVideoDecodeAccelerator::NotifyFlushDone() {
  DVLOGF(2);
  DCHECK(client_);
  client_->NotifyFlushDone();
}

void GpuArcVideoDecodeAccelerator::NotifyResetDone() {
  DVLOGF(2);
  DCHECK(client_);
  client_->NotifyResetDone();
}

void GpuArcVideoDecodeAccelerator::NotifyError(
    ArcVideoDecodeAccelerator::Result error) {
  DVLOGF(2);
  DCHECK(client_);
  DCHECK_NE(error, ArcVideoDecodeAccelerator::SUCCESS);
  client_->NotifyError(
      static_cast<::arc::mojom::VideoDecodeAccelerator::Result>(error));
}

base::ScopedFD GpuArcVideoDecodeAccelerator::UnwrapFdFromMojoHandle(
    mojo::ScopedHandle handle) {
  DCHECK(client_);
  if (!handle.is_valid()) {
    VLOGF(1) << "Handle is invalid.";
    client_->NotifyError(
        ::arc::mojom::VideoDecodeAccelerator::Result::INVALID_ARGUMENT);
    return base::ScopedFD();
  }

  base::PlatformFile platform_file;
  MojoResult mojo_result =
      mojo::UnwrapPlatformFile(std::move(handle), &platform_file);
  if (mojo_result != MOJO_RESULT_OK) {
    VLOGF(1) << "UnwrapPlatformFile failed: " << mojo_result;
    client_->NotifyError(
        ::arc::mojom::VideoDecodeAccelerator::Result::PLATFORM_FAILURE);
    return base::ScopedFD();
  }

  return base::ScopedFD(platform_file);
}

void GpuArcVideoDecodeAccelerator::Initialize(
    media::VideoCodecProfile profile,
    ::arc::mojom::VideoDecodeClientPtr client,
    const InitializeCallback& callback) {
  DVLOGF(2) << "prfoile = " << profile;
  DCHECK(!client_);
  client_ = std::move(client);
  ArcVideoDecodeAccelerator::Result result =
      accelerator_->Initialize(profile, this);
  callback.Run(
      static_cast<::arc::mojom::VideoDecodeAccelerator::Result>(result));
}

void GpuArcVideoDecodeAccelerator::Decode(
    ::arc::mojom::BitstreamBufferPtr bitstream_buffer) {
  DVLOGF(2);
  base::ScopedFD fd =
      UnwrapFdFromMojoHandle(std::move(bitstream_buffer->ashmem_fd));
  chromeos::arc::BitstreamBuffer buffer;
  buffer.bitstream_id = bitstream_buffer->bitstream_id;
  buffer.ashmem_fd = std::move(fd);
  buffer.offset = bitstream_buffer->offset;
  buffer.bytes_used = bitstream_buffer->bytes_used;
  accelerator_->Decode(std::move(buffer));
}

void GpuArcVideoDecodeAccelerator::AssignPictureBuffers(uint32_t count) {
  DVLOGF(2);
  accelerator_->AssignPictureBuffers(count);
}

void GpuArcVideoDecodeAccelerator::ImportBufferForPicture(
    int32_t picture_id,
    mojo::ScopedHandle dmabuf_handle,
    std::vector<::arc::VideoFramePlane> planes) {
  DVLOGF(2);
  base::ScopedFD fd = UnwrapFdFromMojoHandle(std::move(dmabuf_handle));
  if (!fd.is_valid()) {
    return;
  }
  accelerator_->ImportBufferForPicture(picture_id, std::move(fd),
                                       std::move(planes));
}

void GpuArcVideoDecodeAccelerator::ReusePictureBuffer(int32_t picture_id) {
  DVLOGF(2);
  accelerator_->ReusePictureBuffer(picture_id);
}

void GpuArcVideoDecodeAccelerator::Flush() {
  DVLOGF(2);
  accelerator_->Flush();
}

void GpuArcVideoDecodeAccelerator::Reset() {
  DVLOGF(2);
  accelerator_->Reset();
}

}  // namespace arc
}  // namespace chromeos
