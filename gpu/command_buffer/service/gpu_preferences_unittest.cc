// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <cstring>

#include "build/build_config.h"
#include "gpu/command_buffer/service/gpu_preferences.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace gpu {

TEST(GpuPreferencesTest, EncodeDecode) {
  {  // Testing default values.
    GpuPreferences input_prefs, decoded_prefs;
    std::string encoded = input_prefs.ToString();
    bool flag = GpuPreferences::FromString(encoded, &decoded_prefs);
    EXPECT_TRUE(flag);
    EXPECT_EQ(0, memcmp(&input_prefs, &decoded_prefs, sizeof(input_prefs)));
  }

  {  // Randomly change some fields.
    GpuPreferences input_prefs, decoded_prefs;
    input_prefs.enable_gpu_scheduler = true;
    input_prefs.disable_gpu_program_cache = true;
    input_prefs.force_gpu_mem_available = 4096;
#if defined(OS_WIN)
    input_prefs.enable_accelerated_vpx_decode = GpuPreferences::VPX_VENDOR_AMD;
#endif
    std::string encoded = input_prefs.ToString();
    bool flag = GpuPreferences::FromString(encoded, &decoded_prefs);
    EXPECT_TRUE(flag);
    EXPECT_EQ(0, memcmp(&input_prefs, &decoded_prefs, sizeof(input_prefs)));
  }
}

}  // namespace gpu
