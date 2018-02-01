// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <tuple>

#include "base/memory/eventually_read_only_shared_memory_region.h"
#include "base/memory/read_only_shared_memory_region.h"
#include "build/build_config.h"
#include "testing/gtest/include/gtest/gtest.h"

#if defined(OS_POSIX)
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#if defined(OS_LINUX)
#include <sys/syscall.h>
#endif

#if defined(OS_WIN)
#include "base/win/scoped_handle.h"
#endif

#if defined(OS_FUCHSIA)
#include <zircon/process.h>
#include <zircon/syscalls.h>
#include "base/fuchsia/scoped_zx_handle.h"
#endif

namespace base {

const int kRegionSize = 1024;

bool IsMemoryFilledWithByte(const void* memory, size_t size, char byte) {
  // Verify the second mapping still has the right data.
  const char* start_ptr = static_cast<const char*>(memory);
  const char* end_ptr = start_ptr + size;
  for (const char* ptr = start_ptr; ptr < end_ptr; ++ptr) {
    if (*ptr != byte)
      return false;
  }

  return true;
}

void ExpectHandleIsNotWritable(SharedMemoryHandle handle, size_t region_size) {
#if defined(OS_ANDROID)
  int handle_fd = handle.GetHandle();
  errno = 0;
  void* writable = mmap(nullptr, region_size, PROT_READ | PROT_WRITE,
                        MAP_SHARED, handle_fd, 0);
  int mmap_errno = errno;
  EXPECT_EQ(MAP_FAILED, writable)
      << "It shouldn't be possible to re-mmap the descriptor writable.";
  EXPECT_EQ(EPERM, mmap_errno) << strerror(mmap_errno);
  if (writable != MAP_FAILED)
    EXPECT_EQ(0, munmap(writable, region_size));
#elif defined(OS_FUCHSIA)
  uintptr_t addr;
  EXPECT_NE(ZX_OK, zx_vmar_map(zx_vmar_root_self(), 0, handle.GetHandle(), 0,
                               region_size, ZX_VM_FLAG_PERM_WRITE, &addr))
      << "Shouldn't be able to map as writable.";

  ScopedZxHandle duped_handle;
  EXPECT_NE(ZX_OK, zx_handle_duplicate(handle.GetHandle(), ZX_RIGHT_WRITE,
                                       duped_handle.receive()))
      << "Shouldn't be able to duplicate the handle into a writable one.";

  EXPECT_EQ(ZX_OK, zx_handle_duplicate(handle.GetHandle(), ZX_RIGHT_READ,
                                       duped_handle.receive()))
      << "Should be able to duplicate the handle into a readable one.";
#elif defined(OS_POSIX)
  int handle_fd = handle.GetHandle();
  EXPECT_EQ(O_RDONLY, fcntl(handle_fd, F_GETFL) & O_ACCMODE)
      << "The descriptor itself should be read-only.";

  errno = 0;
  void* writable = mmap(nullptr, region_size, PROT_READ | PROT_WRITE,
                        MAP_SHARED, handle_fd, 0);
  int mmap_errno = errno;
  EXPECT_EQ(MAP_FAILED, writable)
      << "It shouldn't be possible to re-mmap the descriptor writable.";
  EXPECT_EQ(EACCES, mmap_errno) << strerror(mmap_errno);
  if (writable != MAP_FAILED)
    EXPECT_EQ(0, munmap(writable, region_size));

#elif defined(OS_WIN)
  EXPECT_EQ(NULL, MapViewOfFile(handle.GetHandle(), FILE_MAP_WRITE, 0, 0, 0))
      << "Shouldn't be able to map memory writable.";

  HANDLE temp_handle;
  BOOL rv = ::DuplicateHandle(GetCurrentProcess(), handle.GetHandle(),
                              GetCurrentProcess(), &temp_handle,
                              FILE_MAP_ALL_ACCESS, false, 0);
  EXPECT_EQ(FALSE, rv)
      << "Shouldn't be able to duplicate the handle into a writable one.";
  if (rv)
    win::ScopedHandle writable_handle(temp_handle);
  rv = ::DuplicateHandle(GetCurrentProcess(), handle.GetHandle(),
                         GetCurrentProcess(), &temp_handle, FILE_MAP_READ,
                         false, 0);
  EXPECT_EQ(TRUE, rv)
      << "Should be able to duplicate the handle into a readable one.";
  if (rv)
    win::ScopedHandle writable_handle(temp_handle);
#else
#error Unexpected platform; write a test that tries to make 'handle' writable.
#endif
}

TEST(ReadOnlySharedMemoryRegionTest, NonValidRegion) {
  ReadOnlySharedMemoryRegion region;
  EXPECT_FALSE(region.IsValid());
  ReadOnlySharedMemoryMapping mapping = region.Map(kRegionSize);
  EXPECT_FALSE(mapping.IsValid());
  ReadOnlySharedMemoryRegion duplicate = region.Duplicate();
  EXPECT_FALSE(duplicate.IsValid());
}

TEST(ReadOnlySharedMemoryRegionTest, Create) {
  ReadOnlySharedMemoryRegion region;
  WritableSharedMemoryMapping rw_mapping;
  std::tie(region, rw_mapping) =
      ReadOnlySharedMemoryRegion::Create(kRegionSize);
  ReadOnlySharedMemoryMapping ro_mapping = region.Map(kRegionSize);
  EXPECT_TRUE(region.IsValid());
  EXPECT_TRUE(rw_mapping.IsValid());
  EXPECT_TRUE(ro_mapping.IsValid());

  // Write data to the first memory mapping, verify contents of the second.
  memset(rw_mapping.memory(), '1', kRegionSize);
  EXPECT_EQ(memcmp(rw_mapping.memory(), ro_mapping.memory(), kRegionSize), 0);

  rw_mapping.Unmap();
  region.Close();
  EXPECT_TRUE(IsMemoryFilledWithByte(ro_mapping.memory(), kRegionSize, '1'));
}

TEST(ReadOnlySharedMemoryRegionTest, ReadOnlyHandleIsNotWritable) {
  ReadOnlySharedMemoryRegion region;
  WritableSharedMemoryMapping rw_mapping;
  std::tie(region, rw_mapping) =
      ReadOnlySharedMemoryRegion::Create(kRegionSize);
  ExpectHandleIsNotWritable(
      ReadOnlySharedMemoryRegion::TakeHandle(std::move(region)), kRegionSize);
}

TEST(EventuallyReadOnlySharedMemoryRegionTest, Create) {
  EventuallyReadOnlySharedMemoryRegion region =
      EventuallyReadOnlySharedMemoryRegion::Create(kRegionSize);
  EXPECT_TRUE(region.IsValid());
  WritableSharedMemoryMapping rw_mapping = region.Map(kRegionSize);
  EXPECT_TRUE(rw_mapping.IsValid());

  memset(rw_mapping.memory(), '1', kRegionSize);
  EXPECT_TRUE(IsMemoryFilledWithByte(rw_mapping.memory(), kRegionSize, '1'));
}

TEST(EventuallyReadOnlySharedMemoryRegionTest, ConvertToReadOnly) {
  EventuallyReadOnlySharedMemoryRegion region =
      EventuallyReadOnlySharedMemoryRegion::Create(kRegionSize);
  WritableSharedMemoryMapping rw_mapping = region.Map(kRegionSize);
  ReadOnlySharedMemoryRegion ro_region =
      EventuallyReadOnlySharedMemoryRegion::ConvertToReadOnly(
          std::move(region));
  EXPECT_TRUE(ro_region.IsValid());
  ReadOnlySharedMemoryMapping ro_mapping = ro_region.Map(kRegionSize);
  EXPECT_TRUE(ro_mapping.IsValid());

  // Write data to the first memory mapping, verify contents of the second.
  memset(rw_mapping.memory(), '1', kRegionSize);
  EXPECT_EQ(memcmp(rw_mapping.memory(), ro_mapping.memory(), kRegionSize), 0);

  rw_mapping.Unmap();
  EXPECT_TRUE(IsMemoryFilledWithByte(ro_mapping.memory(), kRegionSize, '1'));
}

TEST(EventuallyReadOnlySharedMemoryRegionTest,
     IsNotWritableAfterConvertingToReadOnly) {
  EventuallyReadOnlySharedMemoryRegion region =
      EventuallyReadOnlySharedMemoryRegion::Create(kRegionSize);
  ReadOnlySharedMemoryRegion ro_region =
      EventuallyReadOnlySharedMemoryRegion::ConvertToReadOnly(
          std::move(region));
  ExpectHandleIsNotWritable(
      ReadOnlySharedMemoryRegion::TakeHandle(std::move(ro_region)),
      kRegionSize);
}

};  // namespace base
