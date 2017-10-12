// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/gpu/chrome_arc_video_decode_accelerator_deprecated.h"

#include "base/bits.h"
#include "base/callback_helpers.h"
#include "base/logging.h"
#include "base/metrics/histogram_macros.h"
#include "base/numerics/safe_math.h"
#include "base/run_loop.h"
#include "base/sys_info.h"
#include "base/unguessable_token.h"
#include "chrome/gpu/protected_buffer_manager.h"
#include "media/base/video_frame.h"
#include "media/gpu/format_utils.h"
#include "media/gpu/gpu_video_decode_accelerator_factory.h"

#define VLOGF(level) VLOG(level) << __func__ << "(): "
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

int ChromeArcVideoDecodeAcceleratorDeprecated::client_count_ = 0;

ChromeArcVideoDecodeAcceleratorDeprecated::InputRecord::InputRecord(
    int32_t bitstream_buffer_id,
    uint32_t buffer_index,
    int64_t timestamp)
    : bitstream_buffer_id(bitstream_buffer_id),
      buffer_index(buffer_index),
      timestamp(timestamp) {}

ChromeArcVideoDecodeAcceleratorDeprecated::InputBufferInfo::InputBufferInfo() =
    default;

ChromeArcVideoDecodeAcceleratorDeprecated::InputBufferInfo::InputBufferInfo(
    InputBufferInfo&& other) = default;

ChromeArcVideoDecodeAcceleratorDeprecated::InputBufferInfo::~InputBufferInfo() {
  if (shm_handle.OwnershipPassesToIPC())
    shm_handle.Close();
}

ChromeArcVideoDecodeAcceleratorDeprecated::OutputBufferInfo::
    OutputBufferInfo() = default;

ChromeArcVideoDecodeAcceleratorDeprecated::OutputBufferInfo::OutputBufferInfo(
    OutputBufferInfo&& other) = default;

ChromeArcVideoDecodeAcceleratorDeprecated::OutputBufferInfo::
    ~OutputBufferInfo() {
  if (!gpu_memory_buffer_handle.is_null())
    gfx::CloseGpuMemoryBufferHandle(&gpu_memory_buffer_handle);
}

ChromeArcVideoDecodeAcceleratorDeprecated::
    ChromeArcVideoDecodeAcceleratorDeprecated(
        const gpu::GpuPreferences& gpu_preferences,
        ProtectedBufferManager* protected_buffer_manager)
    : arc_client_(nullptr),
      next_bitstream_buffer_id_(0),
      output_pixel_format_(media::PIXEL_FORMAT_UNKNOWN),
      output_buffer_size_(0),
      requested_num_of_output_buffers_(0),
      gpu_preferences_(gpu_preferences),
      protected_buffer_manager_(protected_buffer_manager) {}

ChromeArcVideoDecodeAcceleratorDeprecated::
    ~ChromeArcVideoDecodeAcceleratorDeprecated() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  if (vda_) {
    client_count_--;
  }
}

ArcVideoDecodeAcceleratorDeprecated::Result
ChromeArcVideoDecodeAcceleratorDeprecated::Initialize(
    const Config& config,
    ArcVideoDecodeAcceleratorDeprecated::Client* client) {
  auto result = InitializeTask(config, client);
  // Report initialization status to UMA.
  UMA_HISTOGRAM_ENUMERATION(
      "Media.ChromeArcVideoDecodeAccelerator.InitializeResult", result,
      RESULT_MAX);
  return result;
}

ArcVideoDecodeAcceleratorDeprecated::Result
ChromeArcVideoDecodeAcceleratorDeprecated::InitializeTask(
    const Config& config,
    ArcVideoDecodeAcceleratorDeprecated::Client* client) {
  VLOGF(2) << "input_pixel_format=" << config.input_pixel_format
           << ", num_input_buffers=" << config.num_input_buffers
           << ", secure_mode=" << config.secure_mode << ")";
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(client);

  if (arc_client_) {
    VLOGF(1) << "Re-Initialize() is not allowed";
    return ILLEGAL_STATE;
  }

  arc_client_ = client;

  if (client_count_ >= kMaxConcurrentClients) {
    VLOGF(1) << "too many clients: " << client_count_;
    return INSUFFICIENT_RESOURCES;
  }

  if (config.secure_mode && !protected_buffer_manager_) {
    VLOGF(1) << "Secure mode unsupported";
    return PLATFORM_FAILURE;
  }

  secure_mode_ = config.secure_mode;

  if (config.num_input_buffers > kMaxBufferCount) {
    VLOGF(1) << "Request too many buffers: " << config.num_input_buffers;
    return INVALID_ARGUMENT;
  }
  input_buffer_info_.resize(config.num_input_buffers);

  media::VideoDecodeAccelerator::Config vda_config;
  switch (config.input_pixel_format) {
    case HAL_PIXEL_FORMAT_H264:
      vda_config.profile = media::H264PROFILE_MAIN;
      break;
    case HAL_PIXEL_FORMAT_VP8:
      vda_config.profile = media::VP8PROFILE_ANY;
      break;
    case HAL_PIXEL_FORMAT_VP9:
      vda_config.profile = media::VP9PROFILE_PROFILE0;
      break;
    default:
      VLOGF(1) << "Unsupported input format: " << config.input_pixel_format;
      return INVALID_ARGUMENT;
  }
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

void ChromeArcVideoDecodeAcceleratorDeprecated::SetNumberOfOutputBuffers(
    size_t number) {
  VLOGF(2) << "number=" << number;
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  if (!vda_) {
    VLOGF(1) << "VDA not initialized";
    return;
  }

  if (number > kMaxBufferCount) {
    VLOGF(1) << "Too many buffers: " << number;
    arc_client_->OnError(INVALID_ARGUMENT);
    return;
  }

  std::vector<media::PictureBuffer> buffers;
  for (size_t id = 0; id < number; ++id) {
    // TODO(owenlin): Make sure the |coded_size| is what we want.
    buffers.push_back(
        media::PictureBuffer(base::checked_cast<int32_t>(id), coded_size_));
  }
  vda_->AssignPictureBuffers(buffers);

  buffers_pending_import_.clear();
  buffers_pending_import_.resize(number);
}

bool ChromeArcVideoDecodeAcceleratorDeprecated::AllocateProtectedBuffer(
    PortType port,
    uint32_t index,
    base::ScopedFD handle_fd,
    size_t size) {
  size_t aligned_size =
      base::bits::Align(size, base::SysInfo::VMAllocationGranularity());
  VLOGF(2) << "port=" << port << " index=" << index
           << " handle=" << handle_fd.get() << " size=" << size
           << " aligned=" << aligned_size;
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  if (!secure_mode_) {
    VLOGF(1) << "Not in secure mode";
    arc_client_->OnError(INVALID_ARGUMENT);
    return false;
  }

  if (!ValidatePortAndIndex(port, index)) {
    arc_client_->OnError(INVALID_ARGUMENT);
    return false;
  }

  if (port == PORT_INPUT) {
    auto protected_shmem =
        protected_buffer_manager_->AllocateProtectedSharedMemory(
            std::move(handle_fd), aligned_size);
    if (!protected_shmem) {
      VLOGF(1) << "Failed allocating protected shared memory";
      return false;
    }

    auto input_info = base::MakeUnique<InputBufferInfo>();
    input_info->shm_handle = protected_shmem->shm_handle();
    input_info->protected_buffer_handle = std::move(protected_shmem);
    input_buffer_info_[index] = std::move(input_info);
  } else if (port == PORT_OUTPUT) {
    auto protected_pixmap =
        protected_buffer_manager_->AllocateProtectedNativePixmap(
            std::move(handle_fd),
            media::VideoPixelFormatToGfxBufferFormat(output_pixel_format_),
            coded_size_);
    if (!protected_pixmap) {
      VLOGF(1) << "Failed allocating a protected pixmap";
      return false;
    }
    auto output_info = base::MakeUnique<OutputBufferInfo>();
    output_info->gpu_memory_buffer_handle.type = gfx::NATIVE_PIXMAP;
    output_info->gpu_memory_buffer_handle.native_pixmap_handle =
        CloneHandleForIPC(protected_pixmap->native_pixmap_handle());
    output_info->protected_buffer_handle = std::move(protected_pixmap);
    buffers_pending_import_[index] = std::move(output_info);
  }

  return true;
}

void ChromeArcVideoDecodeAcceleratorDeprecated::BindSharedMemory(
    PortType port,
    uint32_t index,
    base::ScopedFD ashmem_fd,
    off_t offset,
    size_t length) {
  VLOGF(2) << "port=" << port << ", index=" << index << ", offset=" << offset
           << ", length=" << length;
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  if (secure_mode_) {
    VLOGF(1) << "not allowed in secure mode";
    arc_client_->OnError(INVALID_ARGUMENT);
    return;
  }

  if (!vda_) {
    VLOGF(1) << "VDA not initialized";
    return;
  }

  if (port != PORT_INPUT) {
    VLOGF(1) << "SharedMemory is only supported for input";
    arc_client_->OnError(INVALID_ARGUMENT);
    return;
  }
  if (!ValidatePortAndIndex(port, index)) {
    arc_client_->OnError(INVALID_ARGUMENT);
    return;
  }

  auto input_info = base::MakeUnique<InputBufferInfo>();
  input_info->shm_handle =
      base::SharedMemoryHandle(base::FileDescriptor(ashmem_fd.release(), true),
                               length, base::UnguessableToken::Create());
  DCHECK(input_info->shm_handle.OwnershipPassesToIPC());
  input_info->offset = offset;
  input_buffer_info_[index] = std::move(input_info);
}

bool ChromeArcVideoDecodeAcceleratorDeprecated::VerifyDmabuf(
    const base::ScopedFD& dmabuf_fd,
    const std::vector<::arc::VideoFramePlane>& planes) const {
  size_t num_planes = media::VideoFrame::NumPlanes(output_pixel_format_);
  if (planes.size() != num_planes) {
    VLOGF(1) << "Invalid number of dmabuf planes, received: " << planes.size()
             << ", expected: " << num_planes;
    return false;
  }

  off_t size = lseek(dmabuf_fd.get(), 0, SEEK_END);
  lseek(dmabuf_fd.get(), 0, SEEK_SET);
  if (size < 0) {
    VPLOGF(1) << "Failed to find the size of dmabuf";
    return false;
  }

  size_t i = 0;
  for (const auto& plane : planes) {
    VLOGF(4) << "Plane " << i << ", offset=" << plane.offset
             << ", stride=" << plane.stride;

    size_t rows =
        media::VideoFrame::Rows(i, output_pixel_format_, coded_size_.height());
    base::CheckedNumeric<off_t> current_size(plane.offset);
    current_size += base::CheckMul(plane.stride, rows);

    if (!current_size.IsValid() || current_size.ValueOrDie() > size) {
      VLOGF(1) << "Invalid strides/offsets";
      return false;
    }

    ++i;
  }

  return true;
}

void ChromeArcVideoDecodeAcceleratorDeprecated::BindDmabuf(
    PortType port,
    uint32_t index,
    base::ScopedFD dmabuf_fd,
    const std::vector<::arc::VideoFramePlane>& planes) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  VLOGF(2) << "port=" << port << ", index=" << index
           << ", fd=" << dmabuf_fd.get() << ", planes=" << planes.size();

  if (secure_mode_) {
    VLOGF(1) << "not allowed in secure mode";
    arc_client_->OnError(INVALID_ARGUMENT);
    return;
  }

  if (!vda_) {
    VLOGF(1) << "VDA not initialized";
    return;
  }

  if (port != PORT_OUTPUT) {
    VLOGF(1) << "Dmabuf is only supported for input";
    arc_client_->OnError(INVALID_ARGUMENT);
    return;
  }
  if (!ValidatePortAndIndex(port, index)) {
    arc_client_->OnError(INVALID_ARGUMENT);
    return;
  }
  if (!VerifyDmabuf(dmabuf_fd, planes)) {
    arc_client_->OnError(INVALID_ARGUMENT);
    return;
  }

#if defined(USE_OZONE)
  auto output_info = base::MakeUnique<OutputBufferInfo>();
  output_info->gpu_memory_buffer_handle.type = gfx::NATIVE_PIXMAP;
  output_info->gpu_memory_buffer_handle.native_pixmap_handle.fds.emplace_back(
      base::FileDescriptor(dmabuf_fd.release(), true));
  for (const auto& plane : planes) {
    output_info->gpu_memory_buffer_handle.native_pixmap_handle.planes
        .emplace_back(plane.stride, plane.offset, 0, 0);
  }
  buffers_pending_import_[index] = std::move(output_info);
#else
  arc_client_->OnError(PLATFORM_FAILURE);
  return;
#endif
}

void ChromeArcVideoDecodeAcceleratorDeprecated::UseBuffer(
    PortType port,
    uint32_t index,
    const BufferMetadata& metadata) {
  VLOGF(4) << "port=" << port << ", index=" << index
           << ", metadata=(bytes_used=" << metadata.bytes_used
           << ", timestamp=" << metadata.timestamp << ")";
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  if (!vda_) {
    VLOGF(1) << "VDA not initialized";
    return;
  }
  if (!ValidatePortAndIndex(port, index)) {
    arc_client_->OnError(INVALID_ARGUMENT);
    return;
  }
  switch (port) {
    case PORT_INPUT: {
      auto& input_info = input_buffer_info_[index];
      if (!input_info) {
        VLOGF(1) << "Buffer not initialized";
        arc_client_->OnError(INVALID_ARGUMENT);
        return;
      }

      int32_t bitstream_buffer_id = next_bitstream_buffer_id_;
      // Mask against 30 bits, to avoid (undefined) wraparound on signed
      // integer.
      next_bitstream_buffer_id_ = (next_bitstream_buffer_id_ + 1) & 0x3FFFFFFF;

      auto duplicated_handle = input_info->shm_handle.Duplicate();
      if (!duplicated_handle.IsValid()) {
        arc_client_->OnError(PLATFORM_FAILURE);
        return;
      }

      CreateInputRecord(bitstream_buffer_id, index, metadata.timestamp);
      vda_->Decode(
          media::BitstreamBuffer(bitstream_buffer_id, duplicated_handle,
                                 metadata.bytes_used, input_info->offset));
      break;
    }
    case PORT_OUTPUT: {
      auto& output_info = buffers_pending_import_[index];
      if (!output_info) {
        VLOGF(1) << "Buffer not initialized";
        arc_client_->OnError(INVALID_ARGUMENT);
        return;
      }
      // is_null() is false the first time the buffer is passed to the VDA.
      // In that case, VDA needs to import the buffer first.
      if (!output_info->gpu_memory_buffer_handle.is_null()) {
        vda_->ImportBufferForPicture(index,
                                     output_info->gpu_memory_buffer_handle);
        // VDA takes ownership, so just clear out, don't close the handle.
        output_info->gpu_memory_buffer_handle = gfx::GpuMemoryBufferHandle();
      } else {
        vda_->ReusePictureBuffer(index);
      }
      break;
    }
    default:
      NOTREACHED();
  }
}

void ChromeArcVideoDecodeAcceleratorDeprecated::Reset() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  if (!vda_) {
    VLOGF(1) << "VDA not initialized";
    return;
  }
  vda_->Reset();
}

void ChromeArcVideoDecodeAcceleratorDeprecated::Flush() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  if (!vda_) {
    VLOGF(1) << "VDA not initialized";
    return;
  }
  vda_->Flush();
}

void ChromeArcVideoDecodeAcceleratorDeprecated::ProvidePictureBuffers(
    uint32_t requested_num_of_buffers,
    media::VideoPixelFormat output_pixel_format,
    uint32_t textures_per_buffer,
    const gfx::Size& dimensions,
    uint32_t texture_target) {
  VLOGF(2) << "requested_num_of_buffers=" << requested_num_of_buffers
           << ", dimensions=" << dimensions.ToString();
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  coded_size_ = dimensions;

  // By default, use an empty rect to indicate the visible rectangle is not
  // available.
  visible_rect_ = gfx::Rect();
  if ((output_pixel_format_ != media::PIXEL_FORMAT_UNKNOWN) &&
      (output_pixel_format_ != output_pixel_format)) {
    arc_client_->OnError(PLATFORM_FAILURE);
    return;
  }
  output_pixel_format_ = output_pixel_format;
  requested_num_of_output_buffers_ = requested_num_of_buffers;
  output_buffer_size_ =
      media::VideoFrame::AllocationSize(output_pixel_format_, coded_size_);

  NotifyOutputFormatChanged();
}

void ChromeArcVideoDecodeAcceleratorDeprecated::NotifyOutputFormatChanged() {
  VideoFormat video_format;
  switch (output_pixel_format_) {
    case media::PIXEL_FORMAT_I420:
    case media::PIXEL_FORMAT_YV12:
    case media::PIXEL_FORMAT_NV12:
    case media::PIXEL_FORMAT_NV21:
      // HAL_PIXEL_FORMAT_YCbCr_420_888 is the flexible pixel format in Android
      // which handles all 420 formats, with both orderings of chroma (CbCr and
      // CrCb) as well as planar and semi-planar layouts.
      video_format.pixel_format = HAL_PIXEL_FORMAT_YCbCr_420_888;
      break;
    case media::PIXEL_FORMAT_ARGB:
      video_format.pixel_format = HAL_PIXEL_FORMAT_BGRA_8888;
      break;
    default:
      VLOGF(1) << "Format not supported: " << output_pixel_format_;
      arc_client_->OnError(PLATFORM_FAILURE);
      return;
  }
  video_format.buffer_size = output_buffer_size_;
  video_format.min_num_buffers = requested_num_of_output_buffers_;
  video_format.coded_width = coded_size_.width();
  video_format.coded_height = coded_size_.height();
  video_format.crop_top = visible_rect_.y();
  video_format.crop_left = visible_rect_.x();
  video_format.crop_width = visible_rect_.width();
  video_format.crop_height = visible_rect_.height();
  arc_client_->OnOutputFormatChanged(video_format);
}

void ChromeArcVideoDecodeAcceleratorDeprecated::DismissPictureBuffer(
    int32_t picture_buffer) {
  // no-op
}

void ChromeArcVideoDecodeAcceleratorDeprecated::PictureReady(
    const media::Picture& picture) {
  VLOGF(4) << "picture_buffer_id=" << picture.picture_buffer_id()
           << ", bitstream_buffer_id=" << picture.bitstream_buffer_id();
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  // Handle visible size change.
  if (visible_rect_ != picture.visible_rect()) {
    VLOGF(2) << "visible size changed: " << picture.visible_rect().ToString();
    visible_rect_ = picture.visible_rect();
    NotifyOutputFormatChanged();
  }

  InputRecord* input_record = FindInputRecord(picture.bitstream_buffer_id());
  if (input_record == nullptr) {
    VLOGF(1) << "Cannot find for bitstream buffer id: "
             << picture.bitstream_buffer_id();
    arc_client_->OnError(PLATFORM_FAILURE);
    return;
  }

  BufferMetadata metadata;
  metadata.timestamp = input_record->timestamp;
  metadata.bytes_used = output_buffer_size_;
  arc_client_->OnBufferDone(PORT_OUTPUT, picture.picture_buffer_id(), metadata);
}

void ChromeArcVideoDecodeAcceleratorDeprecated::NotifyEndOfBitstreamBuffer(
    int32_t bitstream_buffer_id) {
  VLOGF(4) << "bitstream_buffer_id=" << bitstream_buffer_id;
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  InputRecord* input_record = FindInputRecord(bitstream_buffer_id);
  if (input_record == nullptr) {
    arc_client_->OnError(PLATFORM_FAILURE);
    return;
  }
  arc_client_->OnBufferDone(PORT_INPUT, input_record->buffer_index,
                            BufferMetadata());
}

void ChromeArcVideoDecodeAcceleratorDeprecated::NotifyFlushDone() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  arc_client_->OnFlushDone();
}

void ChromeArcVideoDecodeAcceleratorDeprecated::NotifyResetDone() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  arc_client_->OnResetDone();
}

static ArcVideoDecodeAcceleratorDeprecated::Result ConvertErrorCode(
    media::VideoDecodeAccelerator::Error error) {
  switch (error) {
    case media::VideoDecodeAccelerator::ILLEGAL_STATE:
      return ArcVideoDecodeAcceleratorDeprecated::ILLEGAL_STATE;
    case media::VideoDecodeAccelerator::INVALID_ARGUMENT:
      return ArcVideoDecodeAcceleratorDeprecated::INVALID_ARGUMENT;
    case media::VideoDecodeAccelerator::UNREADABLE_INPUT:
      return ArcVideoDecodeAcceleratorDeprecated::UNREADABLE_INPUT;
    case media::VideoDecodeAccelerator::PLATFORM_FAILURE:
      return ArcVideoDecodeAcceleratorDeprecated::PLATFORM_FAILURE;
    default:
      VLOGF(1) << "Unknown error: " << error;
      return ArcVideoDecodeAcceleratorDeprecated::PLATFORM_FAILURE;
  }
}

void ChromeArcVideoDecodeAcceleratorDeprecated::NotifyError(
    media::VideoDecodeAccelerator::Error error) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  VLOGF(1) << "Error notified: " << error;
  arc_client_->OnError(ConvertErrorCode(error));
}

void ChromeArcVideoDecodeAcceleratorDeprecated::CreateInputRecord(
    int32_t bitstream_buffer_id,
    uint32_t buffer_index,
    int64_t timestamp) {
  input_records_.push_front(
      InputRecord(bitstream_buffer_id, buffer_index, timestamp));

  // The same value copied from media::GpuVideoDecoder. The input record is
  // needed when the input buffer or the corresponding output buffer are
  // returned from VDA. However there is no guarantee how much buffers will be
  // kept in the VDA. We kept the last |kMaxNumberOfInputRecords| in
  // |input_records_| and drop the others.
  const size_t kMaxNumberOfInputRecords = 128;
  if (input_records_.size() > kMaxNumberOfInputRecords)
    input_records_.pop_back();
}

ChromeArcVideoDecodeAcceleratorDeprecated::InputRecord*
ChromeArcVideoDecodeAcceleratorDeprecated::FindInputRecord(
    int32_t bitstream_buffer_id) {
  for (auto& record : input_records_) {
    if (record.bitstream_buffer_id == bitstream_buffer_id)
      return &record;
  }
  return nullptr;
}

bool ChromeArcVideoDecodeAcceleratorDeprecated::ValidatePortAndIndex(
    PortType port,
    uint32_t index) const {
  switch (port) {
    case PORT_INPUT:
      if (index >= input_buffer_info_.size()) {
        VLOGF(1) << "Invalid index: " << index;
        return false;
      }
      return true;
    case PORT_OUTPUT:
      if (index >= buffers_pending_import_.size()) {
        VLOGF(1) << "Invalid index: " << index;
        return false;
      }
      return true;
    default:
      VLOGF(1) << "Invalid port: " << port;
      return false;
  }
}

}  // namespace arc
}  // namespace chromeos
