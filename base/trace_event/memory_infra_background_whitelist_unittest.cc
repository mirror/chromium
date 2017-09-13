// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/trace_event/memory_infra_background_whitelist.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace base {
namespace trace_event {

const char* const kTestDumpNameWhitelist[] = {"test/heap", "test/hex/0x?",
                                              nullptr};

TEST(MemoryInfraBackgroundWhitelistTest, IsMemoryAllocatorDumpNameWhitelisted) {
  SetAllocatorDumpNameWhitelistForTesting(kTestDumpNameWhitelist);

  // Test whitelisted entries.
  bool is_whitelist = IsMemoryAllocatorDumpNameWhitelisted("test/heap");
  ASSERT_TRUE(is_whitelist);

  // Global dumps should be whitelisted.
  is_whitelist = IsMemoryAllocatorDumpNameWhitelisted("global/13456");
  ASSERT_TRUE(is_whitelist);

  // Random names should not.
  is_whitelist = IsMemoryAllocatorDumpNameWhitelisted("random/heap");
  ASSERT_FALSE(is_whitelist);

  // Check hex processing.
  is_whitelist = IsMemoryAllocatorDumpNameWhitelisted("test/hex/0x13489");
  ASSERT_TRUE(is_whitelist);
}

}  // namespace trace_event
}  // namespace base
