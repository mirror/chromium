// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/gpu/chrome_arc_video_decode_accelerator_new.h"

#include "gpu/config/gpu_driver_bug_workarounds.h"
#include "media/base/video_frame.h"
#include "media/gpu/gpu_video_decode_accelerator_factory.h"

#define VLOGF(level) VLOG(level) << __func__ << "(): "
#define DVLOGF(level) DVLOG(level) << __func__ << "(): "
#define VPLOGF(level) VPLOG(level) << __func__ << "(): "
namespace chromeos {
namespace arc {

namespace {

// An arbitrary chosen limit of the number of buffers. The number of
// buffers used is requested from the untrusted client side.
const size_t kMaxBufferCount = 128;

// Maximum number of concurrent ARC video clients.
// Currently we have no way to know the resources are not enough to create more
// VDA. Arbitrarily chosen a reasonable constant as the limit.
const int kMaxConcurrentClients = 8;

}  // anonymous namespace

// Implementation of the VideoDecodeAccelerator::Client interface.
void ChromeArcVideoDecodeAcceleratorNew::ProvidePictureBuffers(
    uint32_t requested_num_of_buffers,
    media::VideoPixelFormat format,
    uint32_t textures_per_buffer,
    const gfx::Size& dimensions,
    uint32_t texture_target) {
  DVLOGF(2);
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(arc_client_);
  if ((output_pixel_format_ != media::PIXEL_FORMAT_UNKNOWN) &&
      (output_pixel_format_ != format)) {
    VLOGF(1) << "Format is not expected one."
             << " output_pixel_format: " << output_pixel_format_
             << " format: " << format;
    arc_client_->NotifyError(PLATFORM_FAILURE);
    return;
  }
  output_pixel_format_ = format;
  coded_size_ = dimensions;
  arc_client_->ProvidePictureBuffers(requested_num_of_buffers, format,
                                     textures_per_buffer, dimensions,
                                     texture_target);
}

void ChromeArcVideoDecodeAcceleratorNew::DismissPictureBuffer(
    int32_t picture_buffer) {
  DVLOGF(2);
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  // No-op.
}

void ChromeArcVideoDecodeAcceleratorNew::PictureReady(
    const media::Picture& picture) {
  DVLOGF(2);
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(arc_client_);
  arc_client_->PictureReady(picture);
}

void ChromeArcVideoDecodeAcceleratorNew::NotifyEndOfBitstreamBuffer(
    int32_t bitstream_buffer_id) {
  DVLOGF(2);
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(arc_client_);
  arc_client_->NotifyEndOfBitstreamBuffer(bitstream_buffer_id);
}

void ChromeArcVideoDecodeAcceleratorNew::NotifyFlushDone() {
  DVLOGF(2);
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(arc_client_);
  arc_client_->NotifyFlushDone();
}

void ChromeArcVideoDecodeAcceleratorNew::NotifyResetDone() {
  DVLOGF(2);
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(arc_client_);
  arc_client_->NotifyResetDone();
}

void ChromeArcVideoDecodeAcceleratorNew::NotifyError(
    media::VideoDecodeAccelerator::Error error) {
  DVLOGF(2);
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(arc_client_);
  DCHECK_NE(static_cast<int>(error), static_cast<int>(Result::SUCCESS));

  arc_client_->NotifyError(static_cast<Result>(error));
}

int ChromeArcVideoDecodeAcceleratorNew::client_count_ = 0;

ChromeArcVideoDecodeAcceleratorNew::ChromeArcVideoDecodeAcceleratorNew(
    const gpu::GpuPreferences& gpu_preferences)
    : arc_client_(nullptr),
      output_pixel_format_(media::PIXEL_FORMAT_UNKNOWN),
      gpu_preferences_(gpu_preferences) {}

ChromeArcVideoDecodeAcceleratorNew::~ChromeArcVideoDecodeAcceleratorNew() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  if (vda_) {
    client_count_--;
  }
}

ArcVideoDecodeAcceleratorNew::Result
ChromeArcVideoDecodeAcceleratorNew::Initialize(
    media::VideoCodecProfile profile,
    ArcVideoDecodeAcceleratorNew::Client* client) {
  DVLOGF(2);
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(client);
  if (arc_client_) {
    VLOGF(1) << "Re-Initialize() is not allowed.";
    return ILLEGAL_STATE;
  }
  if (client_count_ >= kMaxConcurrentClients) {
    VLOGF(1) << "Reject to Initialize() due to too many clients: "
             << client_count_;
    return INSUFFICIENT_RESOURCES;
  }

  arc_client_ = client;

  media::VideoDecodeAccelerator::Config vda_config(profile);

  vda_config.output_mode =
      media::VideoDecodeAccelerator::Config::OutputMode::IMPORT;

  auto vda_factory = media::GpuVideoDecodeAcceleratorFactory::CreateWithNoGL();
  vda_ = vda_factory->CreateVDA(
      this, vda_config, gpu::GpuDriverBugWorkarounds(), gpu_preferences_);

  if (!vda_) {
    VLOGF(1) << "Failed to create VDA.";
    return PLATFORM_FAILURE;
  }

  client_count_++;
  VLOGF(2) << "Number of concurrent ArcVideoDecodeAccelerator clients: "
           << client_count_;

  return SUCCESS;
}

void ChromeArcVideoDecodeAcceleratorNew::Decode(
    BitstreamBuffer&& bitstream_buffer) {
  DVLOGF(2);
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  if (!vda_) {
    VLOGF(1) << "VDA not initialized.";
    return;
  }

  base::UnguessableToken guid = base::UnguessableToken::Create();
  vda_->Decode(media::BitstreamBuffer(
      bitstream_buffer.bitstream_id,
      base::SharedMemoryHandle(
          base::FileDescriptor(bitstream_buffer.ashmem_fd.release(), true), 0u,
          guid),
      bitstream_buffer.bytes_used, bitstream_buffer.offset));
}

void ChromeArcVideoDecodeAcceleratorNew::AssignPictureBuffers(uint32_t count) {
  DVLOGF(2);
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  if (!vda_) {
    VLOGF(1) << "VDA not initialized.";
    return;
  }
  if (count > kMaxBufferCount) {
    VLOGF(1) << "Too many buffers: " << count;
    arc_client_->NotifyError(INVALID_ARGUMENT);
    return;
  }
  std::vector<media::PictureBuffer> buffers;
  for (uint32_t id = 0; id < count; ++id) {
    // TODO(hiroh): Make sure the |coded_size_| is what we want.
    buffers.push_back(
        media::PictureBuffer(base::checked_cast<int32_t>(id), coded_size_));
  }
  vda_->AssignPictureBuffers(buffers);
}

bool ChromeArcVideoDecodeAcceleratorNew::VerifyDmabuf(
    const base::ScopedFD& dmabuf_fd,
    const std::vector<::arc::VideoFramePlane>& planes) const {
  size_t num_planes = media::VideoFrame::NumPlanes(output_pixel_format_);
  if (planes.size() != num_planes) {
    VLOGF(1) << "Invalid number of dmabuf planes passed: " << planes.size()
             << ", expected: " << num_planes;
    return false;
  }

  off_t size = lseek(dmabuf_fd.get(), 0, SEEK_END);
  lseek(dmabuf_fd.get(), 0, SEEK_SET);
  if (size < 0) {
    VPLOGF(1) << "Fail to find the size of dmabuf.";
    return false;
  }

  size_t i = 0;
  for (const auto& plane : planes) {
    DVLOG(4) << "Plane " << i << ", offset: " << plane.offset
             << ", stride: " << plane.stride;

    size_t rows =
        media::VideoFrame::Rows(i, output_pixel_format_, coded_size_.height());
    base::CheckedNumeric<off_t> current_size(plane.offset);
    current_size += base::CheckMul(plane.stride, rows);

    if (!current_size.IsValid() || current_size.ValueOrDie() > size) {
      VLOGF(1) << "Invalid strides/offsets.";
      return false;
    }

    ++i;
  }

  return true;
}

void ChromeArcVideoDecodeAcceleratorNew::ImportBufferForPicture(
    int32_t picture_id,
    base::ScopedFD ashmem_fd,
    std::vector<::arc::VideoFramePlane> planes) {
  DVLOGF(2);
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  if (!vda_) {
    VLOGF(1) << "VDA not initialized.";
    return;
  }
  if (!VerifyDmabuf(ashmem_fd, planes)) {
    VLOGF(1) << "Failed verifying dmabuf";
    arc_client_->NotifyError(INVALID_ARGUMENT);
    return;
  }

  gfx::GpuMemoryBufferHandle handle;
#if defined(USE_OZONE)
  handle.native_pixmap_handle.fds.push_back(
      base::FileDescriptor(ashmem_fd.release(), true));
  for (const auto& plane : planes) {
    handle.native_pixmap_handle.planes.emplace_back(plane.stride, plane.offset,
                                                    0, 0);
  }
#endif
  vda_->ImportBufferForPicture(picture_id, handle);
}

void ChromeArcVideoDecodeAcceleratorNew::ReusePictureBuffer(
    int32_t picture_id) {
  DVLOGF(2);
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  if (!vda_) {
    VLOGF(1) << "VDA not initialized.";
    return;
  }
  vda_->ReusePictureBuffer(picture_id);
}

void ChromeArcVideoDecodeAcceleratorNew::Reset() {
  DVLOGF(2);
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  if (!vda_) {
    VLOGF(1) << "VDA not initialized.";
    return;
  }
  vda_->Reset();
}

void ChromeArcVideoDecodeAcceleratorNew::Flush() {
  DVLOGF(2);
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  if (!vda_) {
    VLOGF(1) << "VDA not initialized.";
    return;
  }
  vda_->Flush();
}

}  // namespace arc
}  // namespace chromeos
