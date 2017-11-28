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
  kSuccess = 0,
  kUserDataDirFail = 1,
  kFileNotFound = 2,
  kMetadataReadFail = 3,
  kInvalidFormatVersion = 4,
  kArraySizeZero = 5,
  kArrayTooBig = 6,
  kArrayReadFail = 7,
  kArrayNotSorted = 8,
  COUNT
};

// Look up a binary based on the required data points.
// - |whitelist| == true to search the whitelist, false for blacklist.
// - Returns true if match found in specified list.  I.E. Allow/Block if true.
bool IsModuleListed(bool whitelist,
                    const std::string& basename,
                    DWORD image_size,
                    DWORD time_date_stamp);

// Get the full path of the whitelist file used.
std::wstring GetWlFilePathUsed();

// Get the full path of the blacklist file used.
std::wstring GetBlFilePathUsed();

// Initialize internal whitelist from file.
FileStatus InitFromFile();

// Overrides the whitelist and blacklist paths for use by tests.
void OverrideFilePathsForTesting(const std::wstring& new_wl_path,
                                 const std::wstring& new_bl_path);

}  // namespace whitelist

#endif  // CHROME_ELF_WHITELIST_WHITELIST_FILE_H_
