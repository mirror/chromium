// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome_elf/whitelist/whitelist_packed_format.h"

namespace whitelist {

// Subdir relative to install_static::GetUserDataDirectory().
const wchar_t kFileSubdir[] =
    L"\\ThirdPartyModuleList"
#if defined(_WIN64)
    L"64";
#else
    L"32";
#endif

// Packed module data cache file.
const wchar_t kBlFileName[] = L"\\bldata";

uint32_t GetLogEntrySize(uint32_t path_len) {
  // LogEvent.path already has 4 bytes of space due to padding.
  uint32_t extra = (path_len > 3) ? (path_len - 3) : 0;

  // The full entry is 32-bit aligned.  Get the modulo, and leave 0
  // alone.
  uint32_t align = (sizeof(LogEntry) + extra) % 4;
  if (align) {
    // Flip the remainder, for how many bytes were added for alignment.
    align = 4 - align;
  }

  return sizeof(LogEntry) + extra + align;
}

}  // namespace whitelist
