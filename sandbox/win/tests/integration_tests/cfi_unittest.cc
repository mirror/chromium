// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include <windows.h>

#include <imagehlp.h>

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/process/launch.h"
#include "base/test/test_timeouts.h"
#include "base/win/pe_image.h"
#include "base/win/scoped_handle.h"
#include "base/win/windows_version.h"
#include "testing/gtest/include/gtest/gtest.h"

extern "C" IMAGE_DOS_HEADER __ImageBase;

namespace sandbox {
// ASLR must be enabled for CFG to be enabled.  As ASLR is disabled in debug
// builds, so must be CFG.
#if defined(NDEBUG)

// This test analyzes our own running binary to ensure MS CFG is enabled.
// (Not instrumented into our code, just enabled for the process system DLLs.)
TEST(CFGSupportTests, BinaryCheck) {
  ASSERT_EQ(0x5A4D, __ImageBase.e_magic);

  PIMAGE_NT_HEADERS nt_headers = reinterpret_cast<PIMAGE_NT_HEADERS>(
      reinterpret_cast<char*>(&__ImageBase) + __ImageBase.e_lfanew);
  ASSERT_EQ(static_cast<DWORD>(IMAGE_NT_SIGNATURE), nt_headers->Signature);

  ASSERT_EQ(static_cast<WORD>(IMAGE_NT_OPTIONAL_HDR_MAGIC),
            nt_headers->OptionalHeader.Magic);
  PIMAGE_OPTIONAL_HEADER optional_header =
      reinterpret_cast<PIMAGE_OPTIONAL_HEADER>(&nt_headers->OptionalHeader);

  // Check 1: Ensure the CFG DllCharacteristics flag is set.
  ASSERT_TRUE(optional_header->DllCharacteristics &
              IMAGE_DLLCHARACTERISTICS_GUARD_CF);

  PIMAGE_DATA_DIRECTORY load_config =
      &(optional_header->DataDirectory[IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG]);
  PIMAGE_LOAD_CONFIG_DIRECTORY load_config_struct =
      reinterpret_cast<PIMAGE_LOAD_CONFIG_DIRECTORY>(
          reinterpret_cast<char*>(optional_header->ImageBase) +
          load_config->VirtualAddress);

  // Check 2: Ensure the Guard CF function count is > 0.
  ASSERT_TRUE(load_config_struct->GuardCFFunctionCount > 0);
  ASSERT_TRUE(load_config_struct->GuardFlags & IMAGE_GUARD_CF_INSTRUMENTED);

  return;
}

#endif  // defined(NDEBUG)
}  // namespace sandbox
