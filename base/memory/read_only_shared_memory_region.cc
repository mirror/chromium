// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/memory/read_only_shared_memory_region.h"

#include <fcntl.h>
#include <sys/mman.h>

#include "base/files/file_util.h"
#include "base/scoped_generic.h"
#include "base/threading/thread_restrictions.h"

namespace base {

struct ScopedPathUnlinkerTraits {
  static const FilePath* InvalidValue() { return nullptr; }

  static void Free(const FilePath* path) {
    if (unlink(path->value().c_str()))
      PLOG(WARNING) << "unlink";
  }
};

// Unlinks the FilePath when the object is destroyed.
using ScopedPathUnlinker =
    ScopedGeneric<const FilePath*, ScopedPathUnlinkerTraits>;

// static
std::pair<ReadOnlySharedMemoryRegion, WritableSharedMemoryMapping>
ReadOnlySharedMemoryRegion::Create(size_t size) {
  if (size == 0)
    return {};

  if (size > static_cast<size_t>(std::numeric_limits<int>::max()))
    return {};

  // This function theoretically can block on the disk, but realistically
  // the temporary files we create will just go into the buffer cache
  // and be deleted before they ever make it out to disk.
  ThreadRestrictions::ScopedAllowIO allow_io;

  ScopedFD fd;
  ScopedFD readonly_fd;
  FilePath path;

  // Q: Why not use the shm_open() etc. APIs?
  // A: Because they're limited to 4mb on OS X.  FFFFFFFUUUUUUUUUUU
  FilePath directory;
  ScopedPathUnlinker path_unlinker;
  ScopedFILE fp;
  if (!GetShmemTempDir(false /* executable */, &directory))
    return {};

  fp.reset(base::CreateAndOpenTemporaryFileInDir(directory, &path));

  if (!fp)
    return {};

  // Deleting the file prevents anyone else from mapping it in (making it
  // private), and prevents the need for cleanup (once the last fd is
  // closed, it is truly freed).
  path_unlinker.reset(&path);

  // Also open as readonly so that we can GetReadOnlyHandle.
  readonly_fd.reset(HANDLE_EINTR(open(path.value().c_str(), O_RDONLY)));
  if (!readonly_fd.is_valid()) {
    DPLOG(ERROR) << "open(\"" << path.value() << "\", O_RDONLY) failed";
    return {};
  }
  fd.reset(fileno(fp.release()));

  if (fd.is_valid()) {
    // Get current size.
    struct stat stat;
    if (fstat(fd.get(), &stat) != 0)
      return {};
    const size_t current_size = stat.st_size;
    if (current_size != size) {
      if (HANDLE_EINTR(ftruncate(fd.get(), size)) != 0)
        return {};
    }
  } else {
    PLOG(ERROR) << "Creating shared memory in " << path.value() << " failed";
    FilePath dir = path.DirName();
    if (access(dir.value().c_str(), W_OK | X_OK) < 0) {
      PLOG(ERROR) << "Unable to access(W_OK|X_OK) " << dir.value();
      if (dir.value() == "/dev/shm") {
        LOG(FATAL) << "This is frequently caused by incorrect permissions on "
                   << "/dev/shm.  Try 'sudo chmod 1777 /dev/shm' to fix.";
      }
    }
    return {};
  }

  if (readonly_fd.is_valid()) {
    struct stat st = {};
    if (fstat(fd.get(), &st))
      NOTREACHED();

    struct stat readonly_st = {};
    if (fstat(readonly_fd.get(), &readonly_st))
      NOTREACHED();
    if (st.st_dev != readonly_st.st_dev || st.st_ino != readonly_st.st_ino) {
      LOG(ERROR) << "writable and read-only inodes don't match; bailing";
      return {};
    }
  }

  void* writable_memory =
      mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd.get(), 0);
  bool mmap_succeeded =
      writable_memory && writable_memory != reinterpret_cast<void*>(-1);
  if (!mmap_succeeded)
    return {};

  DCHECK_EQ(0U, reinterpret_cast<uintptr_t>(writable_memory) &
                    (WritableSharedMemoryMapping::MAP_MINIMUM_ALIGNMENT - 1));

  auto token = UnguessableToken::Create();
  auto handle = SharedMemoryHandle(FileDescriptor(readonly_fd.release(), false),
                                   size, token);
  return {ReadOnlySharedMemoryRegion(handle),
          WritableSharedMemoryMapping(writable_memory, size, token)};
}

// static
SharedMemoryHandle ReadOnlySharedMemoryRegion::TakeHandle(
    ReadOnlySharedMemoryRegion&& region) {
  SharedMemoryHandle rv;
  using std::swap;
  swap(rv, region.handle_);
  return rv;
}

ReadOnlySharedMemoryRegion::ReadOnlySharedMemoryRegion() = default;

ReadOnlySharedMemoryRegion::ReadOnlySharedMemoryRegion(
    SharedMemoryHandle handle)
    : handle_(handle) {}

ReadOnlySharedMemoryRegion::ReadOnlySharedMemoryRegion(
    ReadOnlySharedMemoryRegion&& region) {
  using std::swap;
  swap(handle_, region.handle_);
}

ReadOnlySharedMemoryRegion& ReadOnlySharedMemoryRegion::operator=(
    ReadOnlySharedMemoryRegion&& region) {
  using std::swap;
  swap(handle_, region.handle_);
  return *this;
}

ReadOnlySharedMemoryRegion::~ReadOnlySharedMemoryRegion() {
  Close();
}

ReadOnlySharedMemoryRegion ReadOnlySharedMemoryRegion::Duplicate() {
  return ReadOnlySharedMemoryRegion(handle_.Duplicate());
}

ReadOnlySharedMemoryMapping ReadOnlySharedMemoryRegion::Map(size_t bytes) {
  return MapAt(0, bytes);
}

ReadOnlySharedMemoryMapping ReadOnlySharedMemoryRegion::MapAt(off_t offset,
                                                              size_t bytes) {
  if (!handle_.IsValid())
    return {};

  if (bytes > static_cast<size_t>(std::numeric_limits<int>::max()))
    return {};

  void* memory =
      mmap(nullptr, bytes, PROT_READ, MAP_SHARED, handle_.GetHandle(), offset);
  bool mmap_succeeded = memory && memory != reinterpret_cast<void*>(-1);
  if (!mmap_succeeded)
    return {};

  DCHECK_EQ(0U, reinterpret_cast<uintptr_t>(memory) &
                    (ReadOnlySharedMemoryMapping::MAP_MINIMUM_ALIGNMENT - 1));

  return ReadOnlySharedMemoryMapping(memory, bytes, handle_.GetGUID());
}

bool ReadOnlySharedMemoryRegion::IsValid() {
  return handle_.IsValid();
}

void ReadOnlySharedMemoryRegion::Close() {
  if (handle_.IsValid())
    handle_.Close();
  handle_ = SharedMemoryHandle();
}

}  // namespace base
