// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/gpu/gpu_arc_video_decode_accelerator_new.h"

#include "chrome/gpu/chrome_arc_video_decode_accelerator_new.h"
#include "media/base/video_frame.h"
#include "mojo/public/cpp/system/platform_handle.h"

#if !defined(DVLOGF)
#define DVLOGF(level) DVLOG(level) << __func__ << "(): "
#endif  // !defined(DVLOGF)

// Make sure arc::mojom::VideoDecodeAcceleratorNew::Result and
// chromeos::arc::ArcVideoDecodeAcceleratorNew::Result match.
static_assert(
    static_cast<int>(arc::mojom::VideoDecodeAcceleratorNew::Result::SUCCESS) ==
        chromeos::arc::ArcVideoDecodeAcceleratorNew::SUCCESS,
    "enum mismatch");
static_assert(
    static_cast<int>(
        arc::mojom::VideoDecodeAcceleratorNew::Result::ILLEGAL_STATE) ==
        chromeos::arc::ArcVideoDecodeAcceleratorNew::ILLEGAL_STATE,
    "enum mismatch");
static_assert(
    static_cast<int>(
        arc::mojom::VideoDecodeAcceleratorNew::Result::INVALID_ARGUMENT) ==
        chromeos::arc::ArcVideoDecodeAcceleratorNew::INVALID_ARGUMENT,
    "enum mismatch");
static_assert(
    static_cast<int>(
        arc::mojom::VideoDecodeAcceleratorNew::Result::UNREADABLE_INPUT) ==
        chromeos::arc::ArcVideoDecodeAcceleratorNew::UNREADABLE_INPUT,
    "enum mismatch");
static_assert(
    static_cast<int>(
        arc::mojom::VideoDecodeAcceleratorNew::Result::PLATFORM_FAILURE) ==
        chromeos::arc::ArcVideoDecodeAcceleratorNew::PLATFORM_FAILURE,
    "enum mismatch");
static_assert(
    static_cast<int>(arc::mojom::VideoDecodeAcceleratorNew::Result::
                         INSUFFICIENT_RESOURCES) ==
        chromeos::arc::ArcVideoDecodeAcceleratorNew::INSUFFICIENT_RESOURCES,
    "enum mismatch");

namespace mojo {

// template <>
// struct TypeConverter<arc::mojom::PictureBufferFormatPtr,
//                      chromeos::arc::PictureBufferFormat> {
//   static arc::mojom::PictureBufferFormatPtr Convert(uint32_t pixel_format,
//                                                     uint32_t buffer_size,
//                                                     uint32_t min_num_buffers,
//                                                     uint32_t coded_width,
//                                                     uint32_t coded_height) {
//     arc::mojom::PictureBufferFormatPtr result =
//         arc::mojom::PictureBufferFormat::New();
//     result->pixel_format = pixel_format;
//     result->buffer_size = buffer_size;
//     result->min_num_buffers = min_num_buffers;
//     result->coded_width = coded_width;
//     result->coded_height = coded_height;
//     return result;
//   }
// };

// template<>
// struct TypeConverter<chromeos::arc::BitstreamBuffer,
//                      ::arc::mojom::BitstreamBufferPtr> {
//   static chromeos::arc::BitstreamBuffer Convert(
//       const ::arc::mojom::BitstreamBufferPtr& input,
//       base::ScopedFD unWrapedFD) {
//     chromeos::arc::BitstreamBuffer result;
//     result.bitstream_id = input->bitstream_id;
//     result.ashmem_fd = std::move(unWrapedFD);
//     result.offset = input->offset;
//     result.bytes_used = input->bytes_used;
//     return result;
//   }
// };

}  // namespace mojo

namespace chromeos {
namespace arc {

GpuArcVideoDecodeAcceleratorNew::GpuArcVideoDecodeAcceleratorNew(
    const gpu::GpuPreferences& gpu_preferences)
    : gpu_preferences_(gpu_preferences),
      accelerator_(new ChromeArcVideoDecodeAcceleratorNew(gpu_preferences_)) {}

GpuArcVideoDecodeAcceleratorNew::~GpuArcVideoDecodeAcceleratorNew() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
}

static ::arc::mojom::PictureBufferFormatPtr CreatePBFormatPtr(
    uint32_t pixel_format,
    uint32_t buffer_size,
    uint32_t min_num_buffers,
    uint32_t coded_width,
    uint32_t coded_height) {
  ::arc::mojom::PictureBufferFormatPtr result =
      ::arc::mojom::PictureBufferFormat::New();
  result->pixel_format = pixel_format;
  result->buffer_size = buffer_size;
  result->min_num_buffers = min_num_buffers;
  result->coded_width = coded_width;
  result->coded_height = coded_height;
  return result;
}

static chromeos::arc::BitstreamBuffer CreateBitstreamBuffer(
    const ::arc::mojom::BitstreamBufferPtr& input,
    base::ScopedFD unWrapedFD) {
  chromeos::arc::BitstreamBuffer result;
  result.bitstream_id = input->bitstream_id;
  result.ashmem_fd = std::move(unWrapedFD);
  result.offset = input->offset;
  result.bytes_used = input->bytes_used;
  return result;
}

void GpuArcVideoDecodeAcceleratorNew::ProvidePictureBuffers(
    uint32_t requested_num_of_buffers,
    media::VideoPixelFormat format,
    uint32_t textures_per_buffer,
    const gfx::Size& dimensions,
    uint32_t texture_target) {
  DVLOGF(2);
  DCHECK(client_);
  auto PBFPtr = CreatePBFormatPtr(
      static_cast<uint32_t>(format),
      static_cast<uint32_t>(
          media::VideoFrame::AllocationSize(format, dimensions)),
      requested_num_of_buffers, dimensions.width(), dimensions.height());
  client_->ProvidePictureBuffers(std::move(PBFPtr));
}

void GpuArcVideoDecodeAcceleratorNew::PictureReady(
    const media::Picture& picture) {
  DVLOGF(2);
  DCHECK(client_);
  auto picture_buffer_id = picture.picture_buffer_id();
  auto bitstream_buffer_id = picture.bitstream_buffer_id();
  auto crop_rect = picture.visible_rect();
  client_->PictureReady(picture_buffer_id, bitstream_buffer_id, crop_rect);
}

void GpuArcVideoDecodeAcceleratorNew::NotifyEndOfBitstreamBuffer(
    int32_t bitstream_buffer) {
  DVLOGF(2);
  DCHECK(client_);
  client_->NotifyEndOfBitstreamBuffer(bitstream_buffer);
}

void GpuArcVideoDecodeAcceleratorNew::NotifyFlushDone() {
  DVLOGF(2);
  DCHECK(client_);
  client_->NotifyFlushDone();
}

void GpuArcVideoDecodeAcceleratorNew::NotifyResetDone() {
  DVLOGF(2);
  DCHECK(client_);
  client_->NotifyResetDone();
}

void GpuArcVideoDecodeAcceleratorNew::NotifyError(
    ArcVideoDecodeAcceleratorNew::Result error) {
  DVLOGF(2);
  DCHECK(client_);
  DCHECK_NE(error, ArcVideoDecodeAcceleratorNew::SUCCESS);
  client_->NotifyError(
      static_cast<::arc::mojom::VideoDecodeAcceleratorNew::Result>(error));
}

base::ScopedFD GpuArcVideoDecodeAcceleratorNew::UnwrapFdFromMojoHandle(
    mojo::ScopedHandle handle) {
  DCHECK(client_);
  if (!handle.is_valid()) {
    LOG(ERROR) << "handle is invalid";
    client_->NotifyError(
        ::arc::mojom::VideoDecodeAcceleratorNew::Result::INVALID_ARGUMENT);
    return base::ScopedFD();
  }

  base::PlatformFile platform_file;
  MojoResult mojo_result =
      mojo::UnwrapPlatformFile(std::move(handle), &platform_file);
  if (mojo_result != MOJO_RESULT_OK) {
    LOG(ERROR) << "UnwrapPlatformFile failed: " << mojo_result;
    client_->NotifyError(
        ::arc::mojom::VideoDecodeAcceleratorNew::Result::PLATFORM_FAILURE);
    return base::ScopedFD();
  }

  return base::ScopedFD(platform_file);
}

void GpuArcVideoDecodeAcceleratorNew::Initialize(
    media::VideoCodecProfile profile,
    ::arc::mojom::VideoDecodeClientNewPtr client,
    const InitializeCallback& callback) {
  DVLOGF(2) << "prfoile = " << profile;
  DCHECK(!client_);
  client_ = std::move(client);
  ArcVideoDecodeAcceleratorNew::Result result =
      accelerator_->Initialize(profile, this);
  callback.Run(
      static_cast<::arc::mojom::VideoDecodeAcceleratorNew::Result>(result));
}

void GpuArcVideoDecodeAcceleratorNew::Decode(
    ::arc::mojom::BitstreamBufferPtr bitstream_buffer) {
  DVLOGF(2);
  base::ScopedFD fd =
      UnwrapFdFromMojoHandle(std::move(bitstream_buffer->ashmem_fd));
  accelerator_->Decode(CreateBitstreamBuffer(bitstream_buffer, std::move(fd)));
}

void GpuArcVideoDecodeAcceleratorNew::AssignPictureBuffers(uint32_t count) {
  DVLOGF(2);
  accelerator_->AssignPictureBuffers(count);
}

void GpuArcVideoDecodeAcceleratorNew::ImportBufferForPicture(
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

void GpuArcVideoDecodeAcceleratorNew::ReusePictureBuffer(int32_t picture_id) {
  DVLOGF(2);
  accelerator_->ReusePictureBuffer(picture_id);
}

void GpuArcVideoDecodeAcceleratorNew::Flush() {
  DVLOGF(2);
  accelerator_->Flush();
}

void GpuArcVideoDecodeAcceleratorNew::Reset() {
  DVLOGF(2);
  accelerator_->Reset();
}

}  // namespace arc
}  // namespace chromeos
