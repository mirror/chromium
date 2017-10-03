// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_GPU_PROTECTED_BUFFER_MANAGER_H_
#define CHROME_GPU_PROTECTED_BUFFER_MANAGER_H_

#include <map>

#include "base/memory/ref_counted.h"
#include "base/memory/shared_memory.h"
#include "base/memory/weak_ptr.h"
#include "base/synchronization/lock.h"
#include "gpu/ipc/service/protected_gpu_memory_buffer_manager.h"
#include "ui/gfx/gpu_memory_buffer.h"
#include "ui/gfx/native_pixmap.h"

namespace chromeos {
namespace arc {

// A buffer that can be referred to via a handle (a dummy handle), which does
// not provide access to the actual contents of the protected buffer.
class ProtectedBuffer {
 public:
  // Create a buffer that can be referred to via |dummy_handle|, not giving
  // access to the actual buffer contents. |destruction_cb| will be called
  // on destruction.
  ProtectedBuffer(base::OnceClosure destruction_cb,
                  scoped_refptr<gfx::NativePixmap> dummy_handle);
  virtual ~ProtectedBuffer();

  // Return a duplicated GpuMemoryBufferHandle to the protected buffer contents.
  virtual gfx::GpuMemoryBufferHandle DuplicateGpuMemoryBufferHandle() const = 0;

 private:
  base::OnceClosure destruction_cb_;
  scoped_refptr<gfx::NativePixmap> dummy_handle_;

  DISALLOW_COPY_AND_ASSIGN(ProtectedBuffer);
};

// A ProtectedBuffer whose protected contents are stored in shared memory.
class ProtectedSharedMemory : public ProtectedBuffer {
 public:
  ~ProtectedSharedMemory() override;

  // Allocate a ProtectedSharedMemory buffer of |size| bytes.
  static std::unique_ptr<ProtectedSharedMemory> Create(
      base::OnceClosure destruction_cb,
      scoped_refptr<gfx::NativePixmap> dummy_handle,
      size_t size);

  // Return an unowned handle to the underlying shared memory.
  base::SharedMemoryHandle GetHandle() const { return shmem_->handle(); }

  // ProtectedBuffer implementation.
  gfx::GpuMemoryBufferHandle DuplicateGpuMemoryBufferHandle() const override;

 private:
  ProtectedSharedMemory(base::OnceClosure destruction_cb,
                        scoped_refptr<gfx::NativePixmap> dummy_handle);

  std::unique_ptr<base::SharedMemory> shmem_;

  DISALLOW_COPY_AND_ASSIGN(ProtectedSharedMemory);
};

// A ProtectedBuffer whose protected contents are stored in a native pixmap.
class ProtectedNativePixmap : public ProtectedBuffer {
 public:
  ~ProtectedNativePixmap() override;

  // Allocate a ProtectedNativePixmap of |format| and |size|.
  static std::unique_ptr<ProtectedNativePixmap> Create(
      base::OnceClosure destruction_cb,
      scoped_refptr<gfx::NativePixmap> dummy_handle,
      gfx::BufferFormat format,
      const gfx::Size& size);

  // ProtectedBuffer implementation.
  gfx::GpuMemoryBufferHandle DuplicateGpuMemoryBufferHandle() const override;

 private:
  ProtectedNativePixmap(base::OnceClosure destruction_cb,
                        scoped_refptr<gfx::NativePixmap> dummy_handle);

  scoped_refptr<gfx::NativePixmap> pixmap_;

  DISALLOW_COPY_AND_ASSIGN(ProtectedNativePixmap);
};

// Allocates and manages protected buffers.
//
// Stores a map of dummy handles to protected buffer handles, allowing
// lookup of protected buffers via their corresponding dummy handles.
class ProtectedBufferManager : public gpu::ProtectedGpuMemoryBufferManager {
 public:
  // A ProtectedBufferHandle is used to track an instance of a ProtectedBuffer
  // owned by the client(s) which allocated it. When the last client reference
  // is released, the ProtectedBuffer is destroyed, and the callback passed
  // to it is called, resulting in releasing the ProtectedBufferHandle and
  // removing its mapping from the map stored in ProtectedBufferManager.
  class ProtectedBufferHandle {
   public:
    // Create a handle mapping |dummy_handle| to |protected_buffer_handle|.
    ProtectedBufferHandle(
        scoped_refptr<gfx::NativePixmap> dummy_handle,
        const gfx::GpuMemoryBufferHandle& protected_buffer_handle);

    virtual ~ProtectedBufferHandle();

    // Return a shallow copy of the protected handle.
    gfx::GpuMemoryBufferHandle GetBufferHandle() const {
      return protected_buffer_handle_;
    }

   private:
    scoped_refptr<gfx::NativePixmap> dummy_handle_;
    gfx::GpuMemoryBufferHandle protected_buffer_handle_;

    DISALLOW_COPY_AND_ASSIGN(ProtectedBufferHandle);
  };

  ProtectedBufferManager();
  ~ProtectedBufferManager();

  // Allocate a ProtectedSharedMemory buffer of |size| bytes, to be referred to
  // via |dummy_fd| as the dummy handle.
  std::unique_ptr<ProtectedSharedMemory> AllocateProtectedSharedMemory(
      base::ScopedFD dummy_fd,
      size_t size);

  // Allocate a ProtectedNativePixmap buffer of |format| and |size|, to be
  // referred to via |dummy_fd| as the dummy handle.
  std::unique_ptr<ProtectedNativePixmap> AllocateProtectedNativePixmap(
      base::ScopedFD dummy_fd,
      gfx::BufferFormat format,
      const gfx::Size& size);

  // Return a file descriptor backing a ProtectedSharedMemory buffer, referred
  // to via a dummy handle |dummy_fd|.
  base::ScopedFD GetProtectedSharedMemoryFromFd(base::ScopedFD dummy_fd);

  // gpu::ProtectedGpuMemoryBufferManager implementation.
  gfx::GpuMemoryBufferHandle GetProtectedGpuMemoryBufferFor(
      const gfx::GpuMemoryBufferHandle& handle) override;

 private:
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
                                                 uint32_t* id);

  // Removes an entry for given |id| from buffer_map_, to be called when the
  // last reference to the buffer is dropped.
  void RemoveEntry(uint32_t id);

  // Return a duplicated GpuMemoryBufferHandle to the protected buffer for
  // the given |dummy_fd|.
  gfx::GpuMemoryBufferHandle GetGpuMemoryBufferHandleForFd(
      base::ScopedFD dummy_fd);

  // Stores a map of unique ids to their corresponding ProtectedBufferHandles.
  using ProtectedBufferMap =
      std::map<uint32_t, std::unique_ptr<ProtectedBufferHandle>>;
  ProtectedBufferMap buffer_map_;
  base::Lock buffer_map_lock_;

  base::WeakPtr<ProtectedBufferManager> weak_this_;
  base::WeakPtrFactory<ProtectedBufferManager> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(ProtectedBufferManager);
};

}  // namespace arc
}  // namespace chromeos

#endif  // CHROME_GPU_PROTECTED_BUFFER_MANAGER_H_
