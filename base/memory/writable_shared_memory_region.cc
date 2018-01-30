// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/memory/writable_shared_memory_region.h"

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
WritableSharedMemoryRegion WritableSharedMemoryRegion::Create(size_t size) {
  if (size == 0)
    return {};

  if (size > static_cast<size_t>(std::numeric_limits<int>::max()))
    return {};

  // This function theoretically can block on the disk, but realistically
  // the temporary files we create will just go into the buffer cache
  // and be deleted before they ever make it out to disk.
  ThreadRestrictions::ScopedAllowIO allow_io;

  ScopedFD fd;
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

  int mapped_file = HANDLE_EINTR(dup(fd.get()));
  if (mapped_file == -1)
    NOTREACHED() << "Call to dup failed, errno=" << errno;

  return WritableSharedMemoryRegion(SharedMemoryHandle(
      FileDescriptor(mapped_file, false), size, UnguessableToken::Create()));
}

// static
SharedMemoryHandle WritableSharedMemoryRegion::TakeHandle(
    WritableSharedMemoryRegion&& region) {
  SharedMemoryHandle rv;
  using std::swap;
  swap(rv, region.handle_);
  return rv;
}

WritableSharedMemoryRegion::WritableSharedMemoryRegion() = default;

WritableSharedMemoryRegion::WritableSharedMemoryRegion(
    SharedMemoryHandle handle)
    : handle_(handle) {}

WritableSharedMemoryRegion::WritableSharedMemoryRegion(
    WritableSharedMemoryRegion&& region) {
  using std::swap;
  swap(handle_, region.handle_);
}

WritableSharedMemoryRegion& WritableSharedMemoryRegion::operator=(
    WritableSharedMemoryRegion&& region) {
  using std::swap;
  swap(handle_, region.handle_);
  return *this;
}

WritableSharedMemoryRegion::~WritableSharedMemoryRegion() {
  Close();
}

WritableSharedMemoryRegion WritableSharedMemoryRegion::Duplicate() {
  return WritableSharedMemoryRegion(handle_.Duplicate());
}

WritableSharedMemoryMapping WritableSharedMemoryRegion::Map(size_t bytes) {
  return MapAt(0, bytes);
}

WritableSharedMemoryMapping WritableSharedMemoryRegion::MapAt(off_t offset,
                                                              size_t bytes) {
  if (!handle_.IsValid())
    return {};

  if (bytes > static_cast<size_t>(std::numeric_limits<int>::max()))
    return {};

  void* memory = mmap(nullptr, bytes, PROT_READ | PROT_WRITE, MAP_SHARED,
                      handle_.GetHandle(), offset);
  bool mmap_succeeded = memory && memory != reinterpret_cast<void*>(-1);
  if (!mmap_succeeded)
    return {};

  DCHECK_EQ(0U, reinterpret_cast<uintptr_t>(memory) &
                    (WritableSharedMemoryMapping::MAP_MINIMUM_ALIGNMENT - 1));

  return WritableSharedMemoryMapping(memory, bytes, handle_.GetGUID());
}

bool WritableSharedMemoryRegion::IsValid() {
  return handle_.IsValid();
}

void WritableSharedMemoryRegion::Close() {
  if (handle_.IsValid())
    handle_.Close();
  handle_ = SharedMemoryHandle();
}

}  // namespace base
