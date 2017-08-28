// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_CAPTURE_VIDEO_SHARED_MEMORY_BUFFER_TRACKER_H_
#define MEDIA_CAPTURE_VIDEO_SHARED_MEMORY_BUFFER_TRACKER_H_

#include "base/memory/shared_memory.h"
#include "base/synchronization/lock.h"
#include "media/capture/video/video_capture_buffer_handle.h"
#include "media/capture/video/video_capture_buffer_tracker.h"
#include "mojo/public/cpp/system/platform_handle.h"

namespace gfx {
class Size;
}

namespace media {

// Tracker specifics for SharedMemory.
class SharedMemoryBufferTracker final : public VideoCaptureBufferTracker {
 public:
  SharedMemoryBufferTracker();
  ~SharedMemoryBufferTracker() override;

  bool Init(const gfx::Size& dimensions,
            VideoPixelFormat format,
            VideoPixelStorage storage_type) override;

  std::unique_ptr<VideoCaptureBufferHandle> GetMemoryMappedAccess() override;
  mojo::ScopedSharedBufferHandle GetHandleForTransit(bool read_only) override;
  base::SharedMemoryHandle GetNonOwnedSharedMemoryHandleForLegacyIPC() override;

 private:
  // Accessor to mapped memory. While one or more of these exists, the shared
  // memory is mapped. When the last of these is destroyed, the shared memory is
  // unmapped.
  class Handle : public VideoCaptureBufferHandle {
   public:
    explicit Handle(SharedMemoryBufferTracker* owner);
    ~Handle() final;

    size_t mapped_size() const final;
    uint8_t* data() const final;
    const uint8_t* const_data() const final;

   private:
    SharedMemoryBufferTracker* const owner_;
  };

  // Called by SharedMemoryBufferHandle to decrement |map_ref_count_| and, if
  // zero, unmap the memory from the process.
  void OnHandleDestroyed();

  // The memory created to be shared with renderer processes.
  base::SharedMemory shared_memory_;
  size_t mapped_size_;

  // Protects |map_ref_count_|.
  base::Lock map_ref_count_lock_;

  // The number of SharedMemoryBufferHandle instances that are referencing the
  // mapped memory.
  int map_ref_count_;
};

}  // namespace media

#endif  // MEDIA_CAPTURE_VIDEO_SHARED_MEMORY_BUFFER_TRACKER_H_
