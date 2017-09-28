// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/gpu/protected_buffer_manager.h"

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/memory/shared_memory.h"
#include "mojo/public/cpp/system/buffer.h"
#include "mojo/public/cpp/system/platform_handle.h"
#include "ui/gfx/geometry/size.h"
#include "ui/ozone/public/ozone_platform.h"
#include "ui/ozone/public/surface_factory_ozone.h"

#define VLOGF(level) VLOG(level) << __func__ << "(): "

namespace chromeos {
namespace arc {

namespace {
// Size of the pixmap to be used as the dummy handle for protected buffers.
const gfx::Size kDummyBufferSize(32, 32);
}  // namespace

ProtectedBuffer::ProtectedBuffer(base::OnceClosure destruction_cb,
                                 scoped_refptr<gfx::NativePixmap> dummy_handle)
    : destruction_cb_(std::move(destruction_cb)), dummy_handle_(dummy_handle) {}

ProtectedBuffer::~ProtectedBuffer() {
  std::move(destruction_cb_).Run();
}

ProtectedSharedMemory::ProtectedSharedMemory(
    base::OnceClosure destruction_cb,
    scoped_refptr<gfx::NativePixmap> dummy_handle)
    : ProtectedBuffer(std::move(destruction_cb), dummy_handle) {}

ProtectedSharedMemory::~ProtectedSharedMemory() {}

// static
std::unique_ptr<ProtectedSharedMemory> ProtectedSharedMemory::Create(
    base::OnceClosure destruction_cb,
    scoped_refptr<gfx::NativePixmap> dummy_handle,
    size_t size) {
  std::unique_ptr<ProtectedSharedMemory> protected_shmem(
      new ProtectedSharedMemory(std::move(destruction_cb), dummy_handle));

  mojo::ScopedSharedBufferHandle mojo_shared_buffer =
      mojo::SharedBufferHandle::Create(size);
  if (!mojo_shared_buffer->is_valid()) {
    VLOGF(1) << "Failed to allocate shared memory";
    return nullptr;
  }

  base::SharedMemoryHandle shmem_handle;
  MojoResult mojo_result = mojo::UnwrapSharedMemoryHandle(
      std::move(mojo_shared_buffer), &shmem_handle, nullptr, nullptr);
  if (mojo_result != MOJO_RESULT_OK) {
    VLOGF(1) << "Failed to unwrap a mojo shared memory handle";
    return nullptr;
  }

  protected_shmem->shmem_.reset(new base::SharedMemory(shmem_handle, false));
  return protected_shmem;
}

gfx::GpuMemoryBufferHandle
ProtectedSharedMemory::DuplicateGpuMemoryBufferHandle() const {
  gfx::GpuMemoryBufferHandle handle;
  handle.type = gfx::SHARED_MEMORY_BUFFER;
  handle.handle = base::SharedMemory::DuplicateHandle(shmem_->handle());
  // Indicates that the underlying handle must be closed by client.
  handle.handle.SetOwnershipPassesToIPC(true);
  handle.offset = 0;
  handle.stride = 0;

  return handle;
}

ProtectedNativePixmap::ProtectedNativePixmap(
    base::OnceClosure destruction_cb,
    scoped_refptr<gfx::NativePixmap> dummy_handle)
    : ProtectedBuffer(std::move(destruction_cb), dummy_handle) {}

ProtectedNativePixmap::~ProtectedNativePixmap() {}

// static
std::unique_ptr<ProtectedNativePixmap> ProtectedNativePixmap::Create(
    base::OnceClosure destruction_cb,
    scoped_refptr<gfx::NativePixmap> dummy_handle,
    gfx::BufferFormat format,
    const gfx::Size& size) {
  std::unique_ptr<ProtectedNativePixmap> protected_pixmap(
      new ProtectedNativePixmap(std::move(destruction_cb), dummy_handle));

  ui::OzonePlatform* platform = ui::OzonePlatform::GetInstance();
  ui::SurfaceFactoryOzone* factory = platform->GetSurfaceFactoryOzone();
  protected_pixmap->pixmap_ =
      factory->CreateNativePixmap(gfx::kNullAcceleratedWidget, size, format,
                                  gfx::BufferUsage::SCANOUT_VDA_WRITE);

  if (!protected_pixmap->pixmap_) {
    VLOGF(1) << "Failed allocating a native pixmap";
    return nullptr;
  }

  return protected_pixmap;
}

gfx::GpuMemoryBufferHandle
ProtectedNativePixmap::DuplicateGpuMemoryBufferHandle() const {
  gfx::GpuMemoryBufferHandle handle;
  handle.type = gfx::NATIVE_PIXMAP;
  // This duplicates the underlying fds and sets their ownership to handle.
  handle.native_pixmap_handle = pixmap_->ExportHandle();
  return handle;
}

ProtectedBufferManager::ProtectedBufferHandle::ProtectedBufferHandle(
    scoped_refptr<gfx::NativePixmap> dummy_handle,
    const gfx::GpuMemoryBufferHandle& protected_buffer_handle)
    : dummy_handle_(dummy_handle),
      protected_buffer_handle_(protected_buffer_handle) {}

ProtectedBufferManager::ProtectedBufferHandle::~ProtectedBufferHandle() {}

ProtectedBufferManager::ProtectedBufferManager() : weak_factory_(this) {
  VLOGF(2);
  weak_this_ = weak_factory_.GetWeakPtr();
}

ProtectedBufferManager::~ProtectedBufferManager() {
  VLOGF(2);
}

std::unique_ptr<ProtectedSharedMemory>
ProtectedBufferManager::AllocateProtectedSharedMemory(base::ScopedFD dummy_fd,
                                                      size_t size) {
  VLOGF(2) << "dummy_fd: " << dummy_fd.get() << ", size: " << size;

  // Import the |dummy_fd| to produce a unique id for it.
  uint32_t id;
  auto pixmap = ImportDummyFd(std::move(dummy_fd), &id);
  if (!pixmap)
    return nullptr;

  base::AutoLock lock(buffer_map_lock_);

  if (buffer_map_.find(id) != buffer_map_.end()) {
    VLOGF(1) << "A protected buffer for this handle already exists";
    return nullptr;
  }

  // Allocate a protected buffer and associate it with the dummy pixmap.
  // The pixmap needs to be stored to ensure the id remains the same for
  // the entire lifetime of the dummy pixmap.
  auto protected_shmem = ProtectedSharedMemory::Create(
      base::BindOnce(&ProtectedBufferManager::RemoveEntry, weak_this_, id),
      pixmap, size);
  if (!protected_shmem) {
    VLOGF(1) << "Failed allocating a protected shared memory buffer";
    return nullptr;
  }

  // Store a handle to the buffer in the buffer_map_, and return the actual
  // buffer allocated to the client.
  // The handle will be removed from the map once all client references to
  // the buffer are dropped.
  VLOGF(2) << "New protected shared memory buffer, handle id: " << id;
  auto protected_buffer_handle = base::MakeUnique<ProtectedBufferHandle>(
      pixmap, protected_shmem->DuplicateGpuMemoryBufferHandle());

  // This will always succeed as we find() first above.
  buffer_map_.insert(std::make_pair(id, std::move(protected_buffer_handle)));

  return protected_shmem;
}

std::unique_ptr<ProtectedNativePixmap>
ProtectedBufferManager::AllocateProtectedNativePixmap(base::ScopedFD dummy_fd,
                                                      gfx::BufferFormat format,
                                                      const gfx::Size& size) {
  VLOGF(2) << "dummy_fd: " << dummy_fd.get() << ", format: " << (int)format
           << ", size: " << size.ToString();

  // Import the |dummy_fd| to produce a unique id for it.
  uint32_t id = 0;
  auto pixmap = ImportDummyFd(std::move(dummy_fd), &id);
  if (!pixmap)
    return nullptr;

  base::AutoLock lock(buffer_map_lock_);

  if (buffer_map_.find(id) != buffer_map_.end()) {
    VLOGF(1) << "A protected buffer for this handle already exists";
    return nullptr;
  }

  // Allocate a protected buffer and associate it with the dummy pixmap.
  // The pixmap needs to be stored to ensure the id remains the same for
  // the entire lifetime of the dummy pixmap.
  auto protected_pixmap = ProtectedNativePixmap::Create(
      base::BindOnce(&ProtectedBufferManager::RemoveEntry, weak_this_, id),
      pixmap, format, size);
  if (!protected_pixmap) {
    VLOGF(1) << "Failed allocating a protected native pixmap";
    return nullptr;
  }

  // Store a handle to the buffer in the buffer_map_, and return the actual
  // buffer allocated to the client.
  // The handle will be removed from the map once all client references to
  // the buffer are dropped.
  VLOGF(2) << "New protected native pixmap, handle id: " << id;
  auto protected_buffer_handle = base::MakeUnique<ProtectedBufferHandle>(
      pixmap, protected_pixmap->DuplicateGpuMemoryBufferHandle());

  // This will always succeed as we find() first above.
  buffer_map_.insert(std::make_pair(id, std::move(protected_buffer_handle)));

  return protected_pixmap;
}

gfx::GpuMemoryBufferHandle
ProtectedBufferManager::GetProtectedGpuMemoryBufferFor(
    const gfx::GpuMemoryBufferHandle& handle) {
  base::ScopedFD dummy_fd;

  switch (handle.type) {
    case gfx::SHARED_MEMORY_BUFFER:
      dummy_fd.reset(
          base::SharedMemory::DuplicateHandle(handle.handle).GetHandle());
      break;

    case gfx::NATIVE_PIXMAP:
      // Only the first fd is used for lookup.
      if (handle.native_pixmap_handle.fds.empty())
        return gfx::GpuMemoryBufferHandle();

      dummy_fd.reset(HANDLE_EINTR(dup(handle.native_pixmap_handle.fds[0].fd)));
      break;

    default:
      return gfx::GpuMemoryBufferHandle();
  }

  return GetGpuMemoryBufferHandleForFd(std::move(dummy_fd));
}

base::ScopedFD ProtectedBufferManager::GetProtectedSharedMemoryFromFd(
    base::ScopedFD dummy_fd) {
  gfx::GpuMemoryBufferHandle gpu_memory_buffer_handle =
      GetGpuMemoryBufferHandleForFd(std::move(dummy_fd));

  if (gpu_memory_buffer_handle.type != gfx::SHARED_MEMORY_BUFFER) {
    VLOGF(1) << "Not a shared memory handle";

    gfx::CloseGpuMemoryBufferHandle(&gpu_memory_buffer_handle);
    return base::ScopedFD();
  }

  return base::ScopedFD(gpu_memory_buffer_handle.handle.Release());
}

scoped_refptr<gfx::NativePixmap> ProtectedBufferManager::ImportDummyFd(
    base::ScopedFD dummy_fd,
    uint32_t* id) {
  // 0 is an invalid handle id.
  *id = 0;

  // Import dummy_fd to acquire its unique id.
  // CreateNativePixmapFromHandle() takes ownership and will close the handle
  // also on failure.
  gfx::NativePixmapHandle pixmap_handle;
  pixmap_handle.fds.emplace_back(
      base::FileDescriptor(dummy_fd.release(), true));
  pixmap_handle.planes.emplace_back(gfx::NativePixmapPlane());
  ui::OzonePlatform* platform = ui::OzonePlatform::GetInstance();
  ui::SurfaceFactoryOzone* factory = platform->GetSurfaceFactoryOzone();
  scoped_refptr<gfx::NativePixmap> pixmap =
      factory->CreateNativePixmapFromHandle(
          gfx::kNullAcceleratedWidget, kDummyBufferSize, gfx::BufferFormat::R_8,
          gfx::BufferUsage::PROTECTED, pixmap_handle);
  if (!pixmap) {
    VLOGF(1) << "Failed importing dummy handle";
    return nullptr;
  }

  *id = pixmap->GetUniqueId();
  if (*id == 0) {
    VLOGF(1) << "Failed acquiring unique id for handle";
    return nullptr;
  }

  return pixmap;
}

void ProtectedBufferManager::RemoveEntry(uint32_t id) {
  VLOGF(2) << "id: " << id;

  base::AutoLock lock(buffer_map_lock_);
  auto num_erased = buffer_map_.erase(id);
  if (num_erased != 1)
    VLOGF(1) << "No buffer id " << id << " to destroy";
}

gfx::GpuMemoryBufferHandle
ProtectedBufferManager::GetGpuMemoryBufferHandleForFd(base::ScopedFD dummy_fd) {
  uint32_t id = 0;
  auto pixmap = ImportDummyFd(std::move(dummy_fd), &id);

  base::AutoLock lock(buffer_map_lock_);
  const auto& iter = buffer_map_.find(id);
  if (iter == buffer_map_.end())
    return gfx::GpuMemoryBufferHandle();

  return gfx::CloneHandleForIPC(iter->second->GetBufferHandle());
}

}  // namespace arc
}  // namespace chromeos
