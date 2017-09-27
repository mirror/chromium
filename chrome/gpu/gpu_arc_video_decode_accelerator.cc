// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/gpu/gpu_arc_video_decode_accelerator.h"

#include "base/callback_helpers.h"
#include "base/metrics/histogram_macros.h"
#include "gpu/config/gpu_driver_bug_workarounds.h"
#include "media/base/video_frame.h"
#include "media/gpu/gpu_video_decode_accelerator_factory.h"
#include "mojo/public/cpp/system/platform_handle.h"

#define VLOGF(level) VLOG(level) << __func__ << "(): "
#define DVLOGF(level) DVLOG(level) << __func__ << "(): "
#define VPLOGF(level) VPLOG(level) << __func__ << "(): "

// Make sure arc::mojom::VideoDecodeAccelerator::Result and
// media::VideoDecodeAccelerator::Error match.
static_assert(static_cast<int>(
                  arc::mojom::VideoDecodeAccelerator::Result::ILLEGAL_STATE) ==
                  media::VideoDecodeAccelerator::Error::ILLEGAL_STATE,
              "enum mismatch");
static_assert(
    static_cast<int>(
        arc::mojom::VideoDecodeAccelerator::Result::INVALID_ARGUMENT) ==
        media::VideoDecodeAccelerator::Error::INVALID_ARGUMENT,
    "enum mismatch");
static_assert(
    static_cast<int>(
        arc::mojom::VideoDecodeAccelerator::Result::UNREADABLE_INPUT) ==
        media::VideoDecodeAccelerator::Error::UNREADABLE_INPUT,
    "enum mismatch");
static_assert(
    static_cast<int>(
        arc::mojom::VideoDecodeAccelerator::Result::PLATFORM_FAILURE) ==
        media::VideoDecodeAccelerator::Error::PLATFORM_FAILURE,
    "enum mismatch");

namespace chromeos {
namespace arc {

namespace {

// An arbitrarily chosen limit of the number of buffers. The number of
// buffers used is requested from the untrusted client side.
constexpr size_t kMaxBufferCount = 128;

// Maximum number of concurrent ARC video clients.
// Currently we have no way to know the resources are not enough to create more
// VDAs. Arbitrarily chosen a reasonable constant as the limit.
constexpr int kMaxConcurrentClients = 8;

}  // anonymous namespace

// static
size_t GpuArcVideoDecodeAccelerator::client_count_ = 0;

GpuArcVideoDecodeAccelerator::GpuArcVideoDecodeAccelerator(
    const gpu::GpuPreferences& gpu_preferences)
    : gpu_preferences_(gpu_preferences) {}

GpuArcVideoDecodeAccelerator::~GpuArcVideoDecodeAccelerator() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  if (vda_)
    client_count_--;
}

enum class HalPixelFormatExtension : uint32_t {
  // The pixel formats defined in Android but are used here. They are defined
  // in "system/core/include/system/graphics.h"
  HAL_PIXEL_FORMAT_BGRA_8888 = 5,
  HAL_PIXEL_FORMAT_YCbCr_420_888 = 0x23,
};

void GpuArcVideoDecodeAccelerator::ProvidePictureBuffers(
    uint32_t requested_num_of_buffers,
    media::VideoPixelFormat format,
    uint32_t textures_per_buffer,
    const gfx::Size& dimensions,
    uint32_t texture_target) {
  DVLOGF(2);
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(client_);

  if ((output_pixel_format_ != media::PIXEL_FORMAT_UNKNOWN) &&
      (output_pixel_format_ != format)) {
    VLOGF(1) << "Format is not expected one."
             << " output_pixel_format: " << output_pixel_format_
             << " format: " << format;
    client_->NotifyError(
        ::arc::mojom::VideoDecodeAccelerator::Result::PLATFORM_FAILURE);
    return;
  }

  output_pixel_format_ = format;
  coded_size_ = dimensions;

  uint32_t pixel_format;
  switch (format) {
    case media::PIXEL_FORMAT_I420:
    case media::PIXEL_FORMAT_YV12:
    case media::PIXEL_FORMAT_NV12:
    case media::PIXEL_FORMAT_NV21:
      // HAL_PIXEL_FORMAT_YCbCr_420_888 is the flexible pixel format in Android
      // which handles all 420 formats, with both orderings of chroma (CbCr and
      // CrCb) as well as planar and semi-planar layouts.
      pixel_format = static_cast<uint32_t>(
          HalPixelFormatExtension::HAL_PIXEL_FORMAT_YCbCr_420_888);
      break;
    case media::PIXEL_FORMAT_ARGB:
      pixel_format = static_cast<uint32_t>(
          HalPixelFormatExtension::HAL_PIXEL_FORMAT_BGRA_8888);
      break;
    default:
      VLOGF(1) << "Format not supported: " << format;
      client_->NotifyError(
          ::arc::mojom::VideoDecodeAccelerator::Result::PLATFORM_FAILURE);
      return;
  }

  ::arc::mojom::PictureBufferFormatPtr pbf =
      ::arc::mojom::PictureBufferFormat::New();
  pbf->pixel_format = pixel_format;
  pbf->buffer_size = media::VideoFrame::AllocationSize(format, dimensions);
  pbf->min_num_buffers = requested_num_of_buffers;
  pbf->coded_width = dimensions.width();
  pbf->coded_height = dimensions.height();
  client_->ProvidePictureBuffers(std::move(pbf));
}

void GpuArcVideoDecodeAccelerator::DismissPictureBuffer(
    int32_t picture_buffer) {
  DVLOGF(2);
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  // No-op.
}

void GpuArcVideoDecodeAccelerator::PictureReady(const media::Picture& picture) {
  DVLOGF(4);
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(client_);
  ::arc::mojom::PicturePtr pic = ::arc::mojom::Picture::New();
  pic->picture_buffer_id = picture.picture_buffer_id();
  pic->bitstream_id = picture.bitstream_buffer_id();
  pic->crop_rect = picture.visible_rect();
  client_->PictureReady(std::move(pic));
}

void GpuArcVideoDecodeAccelerator::NotifyEndOfBitstreamBuffer(
    int32_t bitstream_buffer_id) {
  DVLOGF(3);
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(client_);
  client_->NotifyEndOfBitstreamBuffer(bitstream_buffer_id);
}

void GpuArcVideoDecodeAccelerator::NotifyFlushDone() {
  DVLOGF(2);
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(current_callback_);
  base::ResetAndReturn(&current_callback_)
      .Run(::arc::mojom::VideoDecodeAccelerator::Result::SUCCESS);
  RunPendingRequests();
}

void GpuArcVideoDecodeAccelerator::NotifyResetDone() {
  DVLOGF(2);
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(current_callback_);
  base::ResetAndReturn(&current_callback_)
      .Run(::arc::mojom::VideoDecodeAccelerator::Result::SUCCESS);
  RunPendingRequests();
}

static ::arc::mojom::VideoDecodeAccelerator::Result ConvertErrorCode(
    media::VideoDecodeAccelerator::Error error) {
  switch (error) {
    case media::VideoDecodeAccelerator::ILLEGAL_STATE:
      return ::arc::mojom::VideoDecodeAccelerator::Result::ILLEGAL_STATE;
    case media::VideoDecodeAccelerator::INVALID_ARGUMENT:
      return ::arc::mojom::VideoDecodeAccelerator::Result::INVALID_ARGUMENT;
    case media::VideoDecodeAccelerator::UNREADABLE_INPUT:
      return ::arc::mojom::VideoDecodeAccelerator::Result::UNREADABLE_INPUT;
    case media::VideoDecodeAccelerator::PLATFORM_FAILURE:
      return ::arc::mojom::VideoDecodeAccelerator::Result::PLATFORM_FAILURE;
    default:
      VLOGF(1) << "Unknown error: " << error;
      return ::arc::mojom::VideoDecodeAccelerator::Result::PLATFORM_FAILURE;
  }
}

void GpuArcVideoDecodeAccelerator::NotifyError(
    media::VideoDecodeAccelerator::Error error) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  VLOGF(1);
  DCHECK(client_);

  auto arc_err = ConvertErrorCode(error);

  if (current_callback_) {
    base::ResetAndReturn(&current_callback_).Run(arc_err);
  }
  on_error_ = true;
  client_->NotifyError(arc_err);
  // Because the current state is ERROR, all the pending
  // requests are invoked and the callback return with an error state.
  RunPendingRequests();
}

void GpuArcVideoDecodeAccelerator::RunPendingRequests() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  CHECK(!current_callback_);
  while (!current_callback_ && !pending_requests_.empty()) {
    std::move(pending_requests_.front()).Run();
    pending_requests_.pop();
  }
}

static base::ScopedFD UnwrapFdFromMojoHandle(
    const ::arc::mojom::VideoDecodeClientPtr& client,
    mojo::ScopedHandle handle) {
  DCHECK(client);
  if (!handle.is_valid()) {
    VLOGF(1) << "Handle is invalid.";
    return base::ScopedFD();
  }

  base::PlatformFile platform_file;
  MojoResult mojo_result =
      mojo::UnwrapPlatformFile(std::move(handle), &platform_file);
  if (mojo_result != MOJO_RESULT_OK) {
    VLOGF(1) << "UnwrapPlatformFile failed: " << mojo_result;
    return base::ScopedFD();
  }

  return base::ScopedFD(platform_file);
}

void GpuArcVideoDecodeAccelerator::Initialize(
    media::VideoCodecProfile profile,
    ::arc::mojom::VideoDecodeClientPtr client,
    const InitializeCallback& callback) {
  VLOGF(2) << "profile = " << profile;
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(!client_);

  auto result = InitializeTask(profile, std::move(client));

  // Report initialization status to UMA.
  // TODO(hiroh): crbug.com/745457.
  //              Not report "Media.ChromeArcVDA.InitializeResult" after
  //              autototests will be updated as not using ChromeArcVDA.
  const int RESULT_MAX = static_cast<int>(
      ::arc::mojom::VideoDecodeAccelerator::Result::RESULT_MAX);
  UMA_HISTOGRAM_ENUMERATION(
      "Media.ChromeArcVideoDecodeAccelerator.InitializeResult",
      static_cast<int>(result), RESULT_MAX);
  UMA_HISTOGRAM_ENUMERATION(
      "Media.GpuArcVideoDecodeAccelerator.InitializeResult",
      static_cast<int>(result), RESULT_MAX);
  callback.Run(result);
}

::arc::mojom::VideoDecodeAccelerator::Result
GpuArcVideoDecodeAccelerator::InitializeTask(
    media::VideoCodecProfile profile,
    ::arc::mojom::VideoDecodeClientPtr client) {
  if (vda_) {
    VLOGF(1) << "Re-initialization not allowed.";
    return ::arc::mojom::VideoDecodeAccelerator::Result::ILLEGAL_STATE;
  }

  if (client_count_ >= kMaxConcurrentClients) {
    VLOGF(1) << "Reject to Initialize() due to too many clients: "
             << client_count_;
    return ::arc::mojom::VideoDecodeAccelerator::Result::INSUFFICIENT_RESOURCES;
  }

  media::VideoDecodeAccelerator::Config vda_config(profile);
  vda_config.output_mode =
      media::VideoDecodeAccelerator::Config::OutputMode::IMPORT;

  auto vda_factory = media::GpuVideoDecodeAcceleratorFactory::CreateWithNoGL();
  vda_ = vda_factory->CreateVDA(
      this, vda_config, gpu::GpuDriverBugWorkarounds(), gpu_preferences_);
  if (!vda_) {
    VLOGF(1) << "Failed to create VDA.";
    return ::arc::mojom::VideoDecodeAccelerator::Result::PLATFORM_FAILURE;
  }

  client_ = std::move(client);
  client_count_++;
  on_error_ = false;
  current_callback_.Reset();
  DVLOGF(2) << "Number of concurrent clients: " << client_count_;
  return ::arc::mojom::VideoDecodeAccelerator::Result::SUCCESS;
}

void GpuArcVideoDecodeAccelerator::Decode(
    ::arc::mojom::BitstreamBufferPtr bitstream_buffer) {
  DVLOGF(4);
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  if (!vda_) {
    VLOGF(1) << "VDA not initialized.";
    return;
  }

  if (on_error_) {
    VLOGF(1) << "GAVDA state: ERROR";
    return;
  }

  if (current_callback_) {
    pending_requests_.push(base::Bind(&GpuArcVideoDecodeAccelerator::Decode,
                                      base::Unretained(this),
                                      base::Passed(&bitstream_buffer)));
    return;
  }

  base::ScopedFD ashmem_fd =
      UnwrapFdFromMojoHandle(client_, std::move(bitstream_buffer->ashmem_fd));
  if (!ashmem_fd.is_valid()) {
    return;
  }
  base::UnguessableToken guid = base::UnguessableToken::Create();

  vda_->Decode(media::BitstreamBuffer(
      bitstream_buffer->bitstream_id,
      base::SharedMemoryHandle(base::FileDescriptor(ashmem_fd.release(), true),
                               0u, guid),
      bitstream_buffer->bytes_used, bitstream_buffer->offset));
}

void GpuArcVideoDecodeAccelerator::AssignPictureBuffers(uint32_t count) {
  DVLOGF(2);
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  if (!vda_) {
    VLOGF(1) << "VDA not initialized.";
    return;
  }
  if (count > kMaxBufferCount) {
    VLOGF(1) << "Too many buffers: " << count;
    client_->NotifyError(
        ::arc::mojom::VideoDecodeAccelerator::Result::INVALID_ARGUMENT);
    return;
  }

  CHECK(base::IsValueInRangeForNumericType<int32_t>(count));

  std::vector<media::PictureBuffer> buffers;
  for (uint32_t id = 0; id < count; ++id) {
    buffers.push_back(
        media::PictureBuffer(static_cast<int32_t>(id), coded_size_));
  }
  vda_->AssignPictureBuffers(buffers);
}

void GpuArcVideoDecodeAccelerator::ImportBufferForPicture(
    int32_t picture_buffer_id,
    mojo::ScopedHandle dmabuf_handle,
    std::vector<::arc::VideoFramePlane> planes) {
  DVLOGF(3);
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  if (!vda_) {
    VLOGF(1) << "VDA not initialized.";
    return;
  }

  base::ScopedFD dmabuf_fd =
      UnwrapFdFromMojoHandle(client_, std::move(dmabuf_handle));
  if (!dmabuf_fd.is_valid()) {
    client_->NotifyError(
        ::arc::mojom::VideoDecodeAccelerator::Result::PLATFORM_FAILURE);
    return;
  }

  if (!VerifyDmabuf(dmabuf_fd.get(), planes)) {
    VLOGF(1) << "Failed verifying dmabuf";
    client_->NotifyError(
        ::arc::mojom::VideoDecodeAccelerator::Result::INVALID_ARGUMENT);
    return;
  }

  gfx::GpuMemoryBufferHandle handle;
#if defined(USE_OZONE)
  handle.native_pixmap_handle.fds.push_back(
      base::FileDescriptor(dmabuf_fd.release(), true));
  for (const auto& plane : planes) {
    handle.native_pixmap_handle.planes.emplace_back(plane.stride, plane.offset,
                                                    0, 0);
  }
#endif

  vda_->ImportBufferForPicture(picture_buffer_id, handle);
}

void GpuArcVideoDecodeAccelerator::ReusePictureBuffer(
    int32_t picture_buffer_id) {
  DVLOGF(4);
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  if (!vda_) {
    VLOGF(1) << "VDA not initialized.";
    return;
  }

  vda_->ReusePictureBuffer(picture_buffer_id);
}

void GpuArcVideoDecodeAccelerator::Flush(const FlushCallback& callback) {
  DVLOGF(2);
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  if (!vda_) {
    VLOGF(1) << "VDA not initialized.";
    // Callback has to called.
    callback.Run(::arc::mojom::VideoDecodeAccelerator::Result::ILLEGAL_STATE);
    return;
  }
  if (on_error_) {
    VLOGF(1) << "GAVDA state: ERROR";
    callback.Run(
        ::arc::mojom::VideoDecodeAccelerator::Result::PLATFORM_FAILURE);
    return;
  }

  if (current_callback_) {
    pending_requests_.push(base::Bind(&GpuArcVideoDecodeAccelerator::Flush,
                                      base::Unretained(this), callback));
    return;
  }

  current_callback_ = callback;
  vda_->Flush();
}

void GpuArcVideoDecodeAccelerator::Reset(const ResetCallback& callback) {
  DVLOGF(2);
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  if (!vda_) {
    VLOGF(1) << "VDA not initialized.";
    callback.Run(::arc::mojom::VideoDecodeAccelerator::Result::ILLEGAL_STATE);
    return;
  }
  if (on_error_) {
    VLOGF(1) << "GAVDA state: ERROR";
    callback.Run(
        ::arc::mojom::VideoDecodeAccelerator::Result::PLATFORM_FAILURE);
    return;
  }
  if (current_callback_) {
    pending_requests_.push(base::Bind(&GpuArcVideoDecodeAccelerator::Reset,
                                      base::Unretained(this), callback));
    return;
  }
  current_callback_ = callback;
  vda_->Reset();
}

bool GpuArcVideoDecodeAccelerator::VerifyDmabuf(
    const int dmabuf_fd,
    const std::vector<::arc::VideoFramePlane>& planes) const {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  size_t num_planes = media::VideoFrame::NumPlanes(output_pixel_format_);
  if (planes.size() != num_planes) {
    VLOGF(1) << "Invalid number of dmabuf planes passed: " << planes.size()
             << ", expected: " << num_planes;
    return false;
  }

  off_t size = lseek(dmabuf_fd, 0, SEEK_END);
  lseek(dmabuf_fd, 0, SEEK_SET);
  if (size < 0) {
    VPLOGF(1) << "Fail to find the size of dmabuf.";
    return false;
  }

  for (size_t i = 0; i < planes.size(); ++i) {
    const auto& plane = planes[i];

    DVLOGF(4) << "Plane " << i << ", offset: " << plane.offset
              << ", stride: " << plane.stride;

    size_t rows =
        media::VideoFrame::Rows(i, output_pixel_format_, coded_size_.height());
    base::CheckedNumeric<off_t> current_size(plane.offset);
    current_size += base::CheckMul(plane.stride, rows);

    if (!current_size.IsValid() || current_size.ValueOrDie() > size) {
      VLOGF(1) << "Invalid strides/offsets.";
      return false;
    }
  }

  return true;
}

}  // namespace arc
}  // namespace chromeos
