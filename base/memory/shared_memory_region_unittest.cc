// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <tuple>

#include "base/memory/read_only_shared_memory_region.h"
#include "base/memory/shared_memory.h"
#include "base/memory/writable_shared_memory_region.h"
#include "base/test/test_shared_memory_util.h"
#include "build/build_config.h"
#include "testing/gtest/include/gtest/gtest.h"

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

// Tests that a default constructed region is invalid and it produces invalid
// mappings.
TEST(ReadOnlySharedMemoryRegionTest, NonValidRegion) {
  ReadOnlySharedMemoryRegion region;
  EXPECT_FALSE(region.IsValid());
  ReadOnlySharedMemoryMapping mapping = region.Map(kRegionSize);
  EXPECT_FALSE(mapping.IsValid());
  ReadOnlySharedMemoryRegion duplicate = region.Clone();
  EXPECT_FALSE(duplicate.IsValid());
}

// Tests creation of a new read-only region and that it produces writable and
// read-only mappings of the same region.
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

// Tests that the platform-specific handle obtained from a
// ReadOnlySharedMemoryRegion cannot be used to perform a writable mapping with
// low-level system APIs like mmap().
TEST(ReadOnlySharedMemoryRegionTest, ReadOnlyHandleIsNotWritable) {
  ReadOnlySharedMemoryRegion region;
  WritableSharedMemoryMapping rw_mapping;
  std::tie(region, rw_mapping) =
      ReadOnlySharedMemoryRegion::Create(kRegionSize);
  EXPECT_TRUE(CheckReadOnlySharedMemoryHandleForTesting(
      ReadOnlySharedMemoryRegion::TakeHandleForSerialization(
          std::move(region))));
}

// Tests creation of a new writable region and that it produces valid writable
// mappings.
TEST(WritableSharedMemoryRegionTest, Create) {
  WritableSharedMemoryRegion region =
      WritableSharedMemoryRegion::Create(kRegionSize);
  WritableSharedMemoryMapping mapping = region.Map(kRegionSize);
  WritableSharedMemoryMapping mapping2 = region.Map(kRegionSize);
  EXPECT_TRUE(region.IsValid());
  EXPECT_TRUE(mapping.IsValid());
  EXPECT_TRUE(mapping2.IsValid());

  memset(mapping.memory(), '1', kRegionSize);
  EXPECT_EQ(memcmp(mapping.memory(), mapping2.memory(), kRegionSize), 0);

  mapping.Unmap();
  region.Close();

  EXPECT_TRUE(IsMemoryFilledWithByte(mapping2.memory(), kRegionSize, '1'));
}

// Tests that WritableSharedMemoryRegion converted to
// ReadOnlySharedMemoryRegion produces valid mappings of the same region.
TEST(WritableSharedMemoryRegionTest, ConvertToReadOnly) {
  WritableSharedMemoryRegion region =
      WritableSharedMemoryRegion::Create(kRegionSize);
  WritableSharedMemoryMapping rw_mapping = region.Map(kRegionSize);
  ReadOnlySharedMemoryRegion ro_region =
      WritableSharedMemoryRegion::ConvertToReadOnly(std::move(region));
  ReadOnlySharedMemoryMapping ro_mapping = ro_region.Map(kRegionSize);
  EXPECT_TRUE(ro_region.IsValid());
  EXPECT_TRUE(ro_mapping.IsValid());

  // Write data to the first memory mapping, verify contents of the second.
  memset(rw_mapping.memory(), '1', kRegionSize);
  EXPECT_EQ(memcmp(rw_mapping.memory(), ro_mapping.memory(), kRegionSize), 0);

  rw_mapping.Unmap();
  ro_region.Close();

  EXPECT_TRUE(IsMemoryFilledWithByte(ro_mapping.memory(), kRegionSize, '1'));
}

// Tests that the platform-specific handle obtained from a
// WritableSharedMemoryRegion converted into
// ReadOnlySharedMemoryRegion cannot be used to perform a writable mapping with
// low-level system APIs.
TEST(WritableSharedMemoryRegionTest, IsNotWritableAfterConvertingToReadOnly) {
  WritableSharedMemoryRegion region =
      WritableSharedMemoryRegion::Create(kRegionSize);
  ReadOnlySharedMemoryRegion ro_region =
      WritableSharedMemoryRegion::ConvertToReadOnly(std::move(region));
  EXPECT_TRUE(CheckReadOnlySharedMemoryHandleForTesting(
      ReadOnlySharedMemoryRegion::TakeHandleForSerialization(
          std::move(ro_region))));
}

};  // namespace base
