// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gfx/gpu_memory_buffer.h"

#include "ui/gfx/generic_shared_memory_id.h"

#include <iomanip>
#include <sstream>

namespace gfx {

GpuMemoryBufferHandle::GpuMemoryBufferHandle() : type(EMPTY_BUFFER), id(0) {}

GpuMemoryBufferHandle::GpuMemoryBufferHandle(
    const GpuMemoryBufferHandle& other) = default;

GpuMemoryBufferHandle::~GpuMemoryBufferHandle() {}

void GpuMemoryBuffer::SetColorSpaceForScanout(
    const gfx::ColorSpace& color_space) {}

GpuMemoryBufferHandle CloneHandleForIPC(
    const GpuMemoryBufferHandle& source_handle) {
  switch (source_handle.type) {
    case gfx::EMPTY_BUFFER:
      NOTREACHED();
      return source_handle;
    case gfx::SHARED_MEMORY_BUFFER: {
      gfx::GpuMemoryBufferHandle handle;
      handle.type = gfx::SHARED_MEMORY_BUFFER;
      handle.id = source_handle.id;
      handle.handle = base::SharedMemory::DuplicateHandle(source_handle.handle);
      handle.offset = source_handle.offset;
      handle.stride = source_handle.stride;
      return handle;
    }
    case gfx::NATIVE_PIXMAP: {
      gfx::GpuMemoryBufferHandle handle;
      handle.type = gfx::NATIVE_PIXMAP;
      handle.id = source_handle.id;
#if defined(OS_LINUX)
      handle.native_pixmap_handle =
          gfx::CloneHandleForIPC(source_handle.native_pixmap_handle);
#endif
      return handle;
    }
    case gfx::MAILBOX_SHARED_BUFFER: {
      gfx::GpuMemoryBufferHandle handle;
      handle.type = gfx::MAILBOX_SHARED_BUFFER;
      handle.id = source_handle.id;
      std::copy(std::begin(source_handle.mailbox_shared_buffer),
                std::end(source_handle.mailbox_shared_buffer),
                std::begin(handle.mailbox_shared_buffer));
      return handle;
    }
    case gfx::ANDROID_HARDWARE_BUFFER: {
      gfx::GpuMemoryBufferHandle handle;
      handle.type = gfx::ANDROID_HARDWARE_BUFFER;
      handle.id = source_handle.id;
#if defined(OS_ANDROID)
      handle.hardware_buffer_handle =
          gfx::CloneHandleForIPC(source_handle.hardware_buffer_handle);
#endif
      return handle;
    }
    case gfx::IO_SURFACE_BUFFER:
      return source_handle;
  }
  return gfx::GpuMemoryBufferHandle();
}

std::string GpuMemoryBufferHandle::MailboxSharedBufferString() const {
  std::ostringstream ss;
  ss << "{";
  for (size_t i = 0; i < sizeof(mailbox_shared_buffer); ++i) {
    ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(*(reinterpret_cast<const uint8_t*>(&mailbox_shared_buffer[i])));
  }
  ss << "}";
  return ss.str();
}

base::trace_event::MemoryAllocatorDumpGuid GpuMemoryBuffer::GetGUIDForTracing(
    uint64_t tracing_process_id) const {
  return gfx::GetGenericSharedGpuMemoryGUIDForTracing(tracing_process_id,
                                                      GetId());
}

}  // namespace gfx
