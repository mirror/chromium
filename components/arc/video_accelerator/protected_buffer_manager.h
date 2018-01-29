// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_ARC_VIDEO_ACCELERATOR_PROTECTED_BUFFER_MANAGER_H_
#define COMPONENTS_ARC_VIDEO_ACCELERATOR_PROTECTED_BUFFER_MANAGER_H_

#include <map>

#include "base/memory/ref_counted.h"
#include "base/memory/shared_memory.h"
#include "base/memory/weak_ptr.h"
#include "base/synchronization/lock.h"
#include "ui/gfx/gpu_memory_buffer.h"
#include "ui/gfx/native_pixmap.h"

namespace arc {

// ProtectedBufferManager is executed in the GPU process.
// This is created only using Create() by ProtectedBufferAllocator.
// The active ProtectedBufferManager must be one at the same time.
class ProtectedBufferManager {
 public:
  ProtectedBufferManager();
  ~ProtectedBufferManager();

  // Called only by ProtectedBufferAllocator.
  // This is successfully done if and only if there is no
  // ProtcetdBufferManager being used by ProtectedBufferAllocator.
  // Return nullptr on failure.
  static std::shared_ptr<ProtectedtBufferManager> Create();

  // Called from GpuArcVideoDecodeAccelerator.
  // Return active ProtectedBuffermanager.
  static ProtectedtBufferManager* const GetInstance();

  // Allocate a ProtectedSharedMemory buffer of |size| bytes, to be referred to
  // via |dummy_fd| as the dummy handle.
  // Return whether or not allocating is successfully done or not.
  bool AllocateProtectedSharedMemory(base::ScopedFD dummy_fd, size_t size);

  // Allocate a ProtectedNativePixmap of |format| and |size|, to be referred to
  // via |dummy_fd| as the dummy handle.
  // Return whether or not allocating is successfully done or not.
  bool AllocateProtectedNativePixmap(base::ScopedFD dummy_fd,
                                     gfx::BufferFormat format,
                                     const gfx::Size& size);

  // Destroy ProtectedBufferHandle or NativePixmap associated with |dummy_fd|.
  // Return false if if there is no such a protected buffer.
  bool DeallocateProtectedBuffer(base::ScopedFD dummy_fd);

  // Return a duplicated SharedMemoryHandle associated with the |dummy_fd|,
  // if one exists, or an invalid handle otherwise.
  base::SharedMemoryHandle GetProtectedSharedMemoryHandleFor(
      base::ScopedFD dummy_fd);

  // Return a duplicated NativePixmapHandle associated with the |dummy_fd|,
  // if one exists, or an empty handle otherwise.
  gfx::NativePixmapHandle GetProtectedNativePixmapHandleFor(
      base::ScopedFD dummy_fd);

  // Return a protected NativePixmap for a dummy |handle|, if one exists, or
  // nullptr otherwise. On success, the |handle| is closed.
  scoped_refptr<gfx::NativePixmap> GetProtectedNativePixmapFor(
      const gfx::NativePixmapHandle& handle);

 private:
  // Used internally to maintain the association between the dummy handle and
  // the underlying buffer.
  class ProtectedBuffer;
  class ProtectedSharedMemory;
  class ProtectedNativePixmap;

  // Imports the |dummy_fd| as a NativePixmap. This returns a unique |id|,
  // which is guaranteed to be the same for all future imports of any fd
  // referring to the buffer to which |dummy_fd| refers to, regardless of
  // whether it is the same fd as the original one, or not, for the lifetime
  // of the buffer.
  //
  // This allows us to have an unambiguous mapping from any fd referring to
  // the same memory buffer to the same unique id.
  //
  // Returns nullptr on failure, in which case the returned id is not valid.
  scoped_refptr<gfx::NativePixmap> ImportDummyFd(base::ScopedFD dummy_fd,
                                                 uint32_t* id) const;

  // Deallocate all the protected buffers managed by ProtectedBufferManager.
  // This is called from this destructor.
  // Destroying the buffers will result in permanently disassociating
  // the |dummy_fd| with the underlying ProtectedBuffer, but may not free
  // the underlying protected memory, which will remain valid as long
  // as any buffers to it are still in use.
  bool DeallocateAllProtectedBuffers();

  static std::weak_ptr<ProtectedBufferManager> protected_buffer_manager_;
  static std::mutex pbm_mutex_;

  // A map of unique ids to the ProtectedBuffers associated with them.
  using ProtectedBufferMap =
      std::map<uint32_t, std::unique_ptr<ProtectedBuffer>>;
  ProtectedBufferMap buffer_map_;
  base::Lock buffer_map_lock_;

  DISALLOW_COPY_AND_ASSIGN(ProtectedBufferManager);
};

}  // namespace arc

#endif  // COMPONENTS_ARC_VIDEO_ACCELERATOR_PROTECTED_BUFFER_MANAGER_H_
