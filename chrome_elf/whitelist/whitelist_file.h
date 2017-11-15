// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_ELF_WHITELIST_WHITELIST_FILE_H_
#define CHROME_ELF_WHITELIST_WHITELIST_FILE_H_

#include "windows.h"

#include <string>

namespace whitelist {

// "static_cast<int>(FileStatus::value)" to access underlying value.
enum class FileStatus {
  kSuccess,
  kUserDataDirFail,
  kFileNotFound,
  kMetadataReadFail,
  kInvalidFormatVersion,
  kArraySizeZero,
  kArrayTooBig,
  ArrayReadFail,
  ArrayNotSorted
};

// Look up a binary based on the required data points.
// - Returns true if match found in whitelist.  I.E. Allow module load.
bool IsModuleWhitelisted(const std::string& basename,
                         DWORD image_size,
                         DWORD time_date_stamp);

// Get the full path of the whitelist file used.
std::wstring GetFilePathUsed();

// Initialize internal whitelist from file.
FileStatus InitFromFile();

// TESTING ONLY
void OverrideFilePathForTesting(const std::wstring& new_path);

}  // namespace whitelist

#endif  // CHROME_ELF_WHITELIST_WHITELIST_FILE_H_
