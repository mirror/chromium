// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/gpu/ipc/client/gpu_jpeg_decode_accelerator_host.h"

#include <stddef.h>

#include "base/bind.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/shared_memory_handle.h"
#include "build/build_config.h"
#include "mojo/public/cpp/system/platform_handle.h"

namespace media {

GpuJpegDecodeAcceleratorHost::GpuJpegDecodeAcceleratorHost(
    scoped_refptr<base::SingleThreadTaskRunner> io_task_runner,
    mojom::GpuJpegDecodeAcceleratorPtrInfo jpeg_decoder_info)
    : io_task_runner_(io_task_runner) {
  jpeg_decoder_.Bind(std::move(jpeg_decoder_info));
}

GpuJpegDecodeAcceleratorHost::~GpuJpegDecodeAcceleratorHost() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

bool GpuJpegDecodeAcceleratorHost::Initialize(
    JpegDecodeAccelerator::Client* client) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  bool succeeded = false;
  jpeg_decoder_->Initialize(&succeeded);

  if (succeeded)
    client_ = client;

  return succeeded;
}

void GpuJpegDecodeAcceleratorHost::OnDecodeAckOnIOThread(
    int32_t bitstream_buffer_id,
    mojom::Error error) {
  DCHECK(io_task_runner_->BelongsToCurrentThread());

  if (!client_)
    return;

  if (error == mojom::Error::NO_ERRORS) {
    client_->VideoFrameReady(bitstream_buffer_id);
  } else {
    // Only NotifyError once.
    // Client::NotifyError() may trigger deletion of |this| (on another
    // thread), so calling it needs to be the last thing done on this stack!
    JpegDecodeAccelerator::Client* client = nullptr;
    std::swap(client, client_);
    client->NotifyError(bitstream_buffer_id,
                        static_cast<JpegDecodeAccelerator::Error>(error));
  }
}

void GpuJpegDecodeAcceleratorHost::OnDecodeAck(int32_t bitstream_buffer_id,
                                               mojom::Error error) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  io_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&GpuJpegDecodeAcceleratorHost::OnDecodeAckOnIOThread,
                 base::Unretained(this), bitstream_buffer_id, error));
}

void GpuJpegDecodeAcceleratorHost::Decode(
    const BitstreamBuffer& bitstream_buffer,
    const scoped_refptr<VideoFrame>& video_frame) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  DCHECK(
      base::SharedMemory::IsHandleValid(video_frame->shared_memory_handle()));

  base::SharedMemoryHandle input_handle =
      base::SharedMemory::DuplicateHandle(bitstream_buffer.handle());
  if (!base::SharedMemory::IsHandleValid(input_handle)) {
    DLOG(ERROR) << "Failed to duplicate handle of BitstreamBuffer";
    return;
  }

  base::SharedMemoryHandle output_handle =
      base::SharedMemory::DuplicateHandle(video_frame->shared_memory_handle());
  if (!base::SharedMemory::IsHandleValid(output_handle)) {
    DLOG(ERROR) << "Failed to duplicate handle of VideoFrame";
#if defined(OS_POSIX) && !defined(OS_MACOSX)
    if (input_handle.OwnershipPassesToIPC()) {
      input_handle.Close();
    }
#else
// TODO(kcwu) fix the handle leak after crbug.com/493414 resolved.
#endif
    return;
  }

  mojo::ScopedSharedBufferHandle input_buffer_handle =
      mojo::WrapSharedMemoryHandle(input_handle, bitstream_buffer.size(),
                                   true /* read_only */);
  mojom::BitstreamBufferPtr buffer = mojom::BitstreamBuffer::New();
  buffer->id = bitstream_buffer.id();
  buffer->memory_handle = std::move(input_buffer_handle);
  buffer->size = bitstream_buffer.size();
  buffer->offset = bitstream_buffer.offset();
  buffer->timestamp = bitstream_buffer.presentation_timestamp();
  buffer->key_id = bitstream_buffer.key_id();
  buffer->iv = bitstream_buffer.iv();
  buffer->subsamples = bitstream_buffer.subsamples();

  size_t output_buffer_size = VideoFrame::AllocationSize(
      video_frame->format(), video_frame->coded_size());
  mojo::ScopedSharedBufferHandle output_frame_handle =
      mojo::WrapSharedMemoryHandle(output_handle, output_buffer_size,
                                   false /* read_only */);

  jpeg_decoder_->Decode(std::move(buffer), video_frame->coded_size(),
                        std::move(output_frame_handle),
                        base::checked_cast<uint32_t>(output_buffer_size),
                        base::Bind(&GpuJpegDecodeAcceleratorHost::OnDecodeAck,
                                   base::Unretained(this)));
}

bool GpuJpegDecodeAcceleratorHost::IsSupported() {
  return true;
}

}  // namespace media
