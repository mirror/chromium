// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/memory/read_only_shared_memory_region.h"

#include <tuple>

#include "testing/gtest/include/gtest/gtest.h"

namespace base {

const int kRegionSize = 1024;

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

  // Make sure we don't segfault.
  ASSERT_NE(rw_mapping.memory(), static_cast<void*>(nullptr));
  ASSERT_NE(ro_mapping.memory(), static_cast<void*>(nullptr));

  // Write data to the first memory mapping, verify contents of the second.
  memset(rw_mapping.memory(), '1', kRegionSize);
  EXPECT_EQ(memcmp(rw_mapping.memory(), ro_mapping.memory(), kRegionSize), 0);

  rw_mapping.Unmap();
  region.Close();

  const char* start_ptr = static_cast<const char*>(ro_mapping.memory());
  const char* end_ptr = start_ptr + kRegionSize;
  for (const char* ptr = start_ptr; ptr < end_ptr; ++ptr)
    EXPECT_EQ(*ptr, '1');
}

};  // namespace base
