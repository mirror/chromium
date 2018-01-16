// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_ELF_WHITELIST_WHITELIST_LOG_H_
#define CHROME_ELF_WHITELIST_WHITELIST_LOG_H_

#include "windows.h"

#include <string>

#include <stdint.h>

namespace whitelist {

// "static_cast<int>(LogStatus::value)" to access underlying value.
enum class LogStatus { kSuccess = 0, kCreateMutexFailure = 1, COUNT };

// LogLoadAttempt should be called for all module load attempts seen.
// - Hashes are expected to be of size elf_sha1::kSHA1Length.
// - nullptr should be passed for |full_image_path| if |blocked| == true,
//   as it will be ignored.
void LogLoadAttempt(bool blocked,
                    const uint8_t* basename_hash,
                    const uint8_t* code_id_hash,
                    const char* full_image_path);

// Initialize internal logs.
LogStatus InitLogs();

}  // namespace whitelist

#endif  // CHROME_ELF_WHITELIST_WHITELIST_LOG_H_
