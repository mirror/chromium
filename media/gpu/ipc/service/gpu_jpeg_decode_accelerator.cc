// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/gpu/ipc/service/gpu_jpeg_decode_accelerator.h"

#include <stdint.h>

#include <memory>
#include <utility>

#include "base/command_line.h"
#include "base/containers/hash_tables.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/memory/shared_memory.h"
#include "base/single_thread_task_runner.h"
#include "base/trace_event/trace_event.h"
#include "build/build_config.h"
#include "media/base/bind_to_current_loop.h"
#include "media/base/media_switches.h"
#include "media/filters/jpeg_parser.h"
#include "media/gpu/fake_jpeg_decode_accelerator.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "mojo/public/cpp/system/platform_handle.h"
#include "ui/gfx/geometry/size.h"

#if defined(OS_CHROMEOS)
#if defined(ARCH_CPU_X86_FAMILY)
#include "media/gpu/vaapi_jpeg_decode_accelerator.h"
#endif
#if defined(USE_V4L2_CODEC) && defined(ARCH_CPU_ARM_FAMILY)
#include "media/gpu/v4l2_device.h"
#include "media/gpu/v4l2_jpeg_decode_accelerator.h"
#endif

#endif

namespace {

std::unique_ptr<media::JpegDecodeAccelerator> CreateV4L2JDA(
    scoped_refptr<base::SingleThreadTaskRunner> io_task_runner) {
  std::unique_ptr<media::JpegDecodeAccelerator> decoder;
#if defined(OS_CHROMEOS) && defined(USE_V4L2_CODEC) && \
    defined(ARCH_CPU_ARM_FAMILY)
  scoped_refptr<media::V4L2Device> device = media::V4L2Device::Create();
  if (device)
    decoder.reset(new media::V4L2JpegDecodeAccelerator(
        device, std::move(io_task_runner)));
#endif
  return decoder;
}

std::unique_ptr<media::JpegDecodeAccelerator> CreateVaapiJDA(
    scoped_refptr<base::SingleThreadTaskRunner> io_task_runner) {
  std::unique_ptr<media::JpegDecodeAccelerator> decoder;
#if defined(OS_CHROMEOS) && defined(ARCH_CPU_X86_FAMILY)
  decoder.reset(
      new media::VaapiJpegDecodeAccelerator(std::move(io_task_runner)));
#endif
  return decoder;
}

std::unique_ptr<media::JpegDecodeAccelerator> CreateFakeJDA(
    scoped_refptr<base::SingleThreadTaskRunner> io_task_runner) {
  return base::MakeUnique<media::FakeJpegDecodeAccelerator>(
      std::move(io_task_runner));
}

void DecodeFinished(std::unique_ptr<base::SharedMemory> shm) {
  // Do nothing. Because VideoFrame is backed by |shm|, the purpose of this
  // function is to just keep reference of |shm| to make sure it lives until
  // decode finishes.
}

bool VerifyDecodeParams(const gfx::Size& coded_size,
                        mojo::ScopedSharedBufferHandle* output_handle,
                        uint32_t output_buffer_size) {
  const int kJpegMaxDimension = UINT16_MAX;
  if (coded_size.IsEmpty() || coded_size.width() > kJpegMaxDimension ||
      coded_size.height() > kJpegMaxDimension) {
    LOG(ERROR) << "invalid coded_size " << coded_size.ToString();
    return false;
  }

  if (!output_handle->is_valid()) {
    LOG(ERROR) << "invalid output_handle";
    return false;
  }

  if (output_buffer_size <
      media::VideoFrame::AllocationSize(media::PIXEL_FORMAT_I420, coded_size)) {
    LOG(ERROR) << "output_buffer_size is too small: " << output_buffer_size;
    return false;
  }

  return true;
}

}  // namespace

namespace media {

// static
bool GpuJpegDecodeAcceleratorFactoryProvider::
    IsAcceleratedJpegDecodeSupported() {
  auto accelerator_factory_functions = GetAcceleratorFactories();
  for (const auto& create_jda_function : accelerator_factory_functions) {
    std::unique_ptr<JpegDecodeAccelerator> accelerator =
        create_jda_function.Run(base::ThreadTaskRunnerHandle::Get());
    if (accelerator && accelerator->IsSupported())
      return true;
  }
  return false;
}

// static
std::vector<GpuJpegDecodeAcceleratorFactoryProvider::CreateAcceleratorCB>
GpuJpegDecodeAcceleratorFactoryProvider::GetAcceleratorFactories() {
  // This list is ordered by priority of use.
  std::vector<CreateAcceleratorCB> result;
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kUseFakeJpegDecodeAccelerator)) {
    result.push_back(base::Bind(&CreateFakeJDA));
  } else {
    result.push_back(base::Bind(&CreateV4L2JDA));
    result.push_back(base::Bind(&CreateVaapiJDA));
  }
  return result;
}

// static
void GpuJpegDecodeAccelerator::Create(
    const service_manager::BindSourceInfo& /*source_info*/,
    mojom::GpuJpegDecodeAcceleratorRequest request) {
  auto* jpeg_decoder = new GpuJpegDecodeAccelerator();
  mojo::MakeStrongBinding(base::WrapUnique(jpeg_decoder), std::move(request));
}

GpuJpegDecodeAccelerator::GpuJpegDecodeAccelerator()
    : accelerator_factory_functions_(
          GpuJpegDecodeAcceleratorFactoryProvider::GetAcceleratorFactories()) {}

GpuJpegDecodeAccelerator::~GpuJpegDecodeAccelerator() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
}

void GpuJpegDecodeAccelerator::VideoFrameReady(int32_t bitstream_buffer_id) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  NotifyDecodeStatus(bitstream_buffer_id, mojom::Error::NO_ERRORS);
}

void GpuJpegDecodeAccelerator::NotifyError(int32_t bitstream_buffer_id,
                                           JpegDecodeAccelerator::Error error) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  // TODO(c.padhi): Add EnumTraits for JpegDecodeAccelerator::Error, see
  // http://crbug.com/732253.
  NotifyDecodeStatus(bitstream_buffer_id, static_cast<mojom::Error>(error));
}

void GpuJpegDecodeAccelerator::Initialize(InitializeCallback callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  // When adding non-chromeos platforms, VideoCaptureGpuJpegDecoder::Initialize
  // needs to be updated.

  std::unique_ptr<JpegDecodeAccelerator> accelerator;
  for (const auto& create_jda_function : accelerator_factory_functions_) {
    std::unique_ptr<JpegDecodeAccelerator> tmp_accelerator =
        create_jda_function.Run(base::ThreadTaskRunnerHandle::Get());
    if (tmp_accelerator && tmp_accelerator->Initialize(this)) {
      accelerator = std::move(tmp_accelerator);
      break;
    }
  }

  if (!accelerator) {
    DLOG(ERROR) << "JPEG accelerator Initialize failed";
    std::move(callback).Run(false);
    return;
  }

  accelerator_ = std::move(accelerator);
  std::move(callback).Run(true);
}

void GpuJpegDecodeAccelerator::Decode(
    mojom::BitstreamBufferPtr input_buffer,
    const gfx::Size& coded_size,
    mojo::ScopedSharedBufferHandle output_handle,
    uint32_t output_buffer_size,
    DecodeCallback callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  TRACE_EVENT0("jpeg", "GpuJpegDecodeAccelerator::Decode");

  decode_cb_ = std::move(callback);

  if (!VerifyDecodeParams(coded_size, &output_handle, output_buffer_size)) {
    NotifyDecodeStatus(input_buffer->id, mojom::Error::INVALID_ARGUMENT);
    return;
  }

  base::SharedMemoryHandle memory_handle;
  size_t memory_size = 0;
  bool read_only_flag = false;

  MojoResult result = mojo::UnwrapSharedMemoryHandle(
      std::move(output_handle), &memory_handle, &memory_size, &read_only_flag);
  DCHECK_EQ(MOJO_RESULT_OK, result);

  std::unique_ptr<base::SharedMemory> output_shm(
      new base::SharedMemory(memory_handle, false));
  if (!output_shm->Map(output_buffer_size)) {
    LOG(ERROR) << "Could not map output shared memory for input buffer id "
               << input_buffer->id;
    NotifyDecodeStatus(input_buffer->id, mojom::Error::PLATFORM_FAILURE);
    return;
  }

  uint8_t* shm_memory = static_cast<uint8_t*>(output_shm->memory());
  scoped_refptr<VideoFrame> frame = VideoFrame::WrapExternalSharedMemory(
      PIXEL_FORMAT_I420,      // format
      coded_size,             // coded_size
      gfx::Rect(coded_size),  // visible_rect
      coded_size,             // natural_size
      shm_memory,             // data
      output_buffer_size,     // data_size
      memory_handle,          // handle
      0,                      // data_offset
      base::TimeDelta());     // timestamp
  if (!frame.get()) {
    LOG(ERROR) << "Could not create VideoFrame for input buffer id "
               << input_buffer->id;
    NotifyDecodeStatus(input_buffer->id, mojom::Error::PLATFORM_FAILURE);
    return;
  }
  frame->AddDestructionObserver(
      base::Bind(DecodeFinished, base::Passed(&output_shm)));

  result = mojo::UnwrapSharedMemoryHandle(
      std::move(input_buffer->memory_handle), &memory_handle, &memory_size,
      &read_only_flag);
  DCHECK_EQ(MOJO_RESULT_OK, result);

  BitstreamBuffer bitstream_buffer(input_buffer->id, memory_handle,
                                   input_buffer->size, input_buffer->offset,
                                   input_buffer->timestamp);

  bitstream_buffer.SetDecryptConfig(DecryptConfig(
      input_buffer->key_id, input_buffer->iv, input_buffer->subsamples));

  DCHECK(accelerator_);
  accelerator_->Decode(bitstream_buffer, frame);
}

void GpuJpegDecodeAccelerator::Uninitialize() {
  // TODO(c.padhi): see http://crbug.com/699255.
  NOTIMPLEMENTED();
}

void GpuJpegDecodeAccelerator::NotifyDecodeStatus(int32_t bitstream_buffer_id,
                                                  mojom::Error error) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(decode_cb_);
  std::move(decode_cb_).Run(bitstream_buffer_id, error);
}

}  // namespace media
