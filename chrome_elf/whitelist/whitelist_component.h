// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_ELF_WHITELIST_WHITELIST_COMPONENT_H_
#define CHROME_ELF_WHITELIST_WHITELIST_COMPONENT_H_

#include "windows.h"

#include <string>

namespace whitelist {

// "static_cast<int>(ComponentStatus::value)" to access underlying value.
enum class ComponentStatus {
  kSuccess = 0,
  kUserDataDirFail,
  kInitArrayFail,
  kNoComponentFile,
};

// Look up a binary based on the required data points.
// - Returns true if match found in whitelist.  I.E. Allow module load.
bool WhitelistLookup(const std::string& basename,
                     DWORD image_size,
                     DWORD time_date_stamp);

// Get the full path of the whitelist component file used.
std::wstring GetComponentPathUsed();

// Initialize internal whitelist from component file.
ComponentStatus InitComponent();

}  // namespace whitelist

#endif  // CHROME_ELF_WHITELIST_WHITELIST_COMPONENT_H_
