// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_ELF_WHITELIST_WHITELIST_DEFERRAL_H_
#define CHROME_ELF_WHITELIST_WHITELIST_DEFERRAL_H_

#include <string>

namespace whitelist {

// "static_cast<int>(DeferralStatus::value)" to access underlying value.
enum class DeferralStatus {
  kSuccess = 0,
};

// Add a full image path to be validated at a later time (deferral).  This is
// used for "unknown" images, that an explicit block or allow is not available
// for.
//
// - This function takes ownership of the std::wstring passed in.
void AddDeferral(std::wstring&& path);

// Get the number of elements currently in the deferral list.
size_t GetDeferralListSize();

// Initialize deferral infrastructure.
DeferralStatus InitDeferrals();

}  // namespace whitelist

#endif  // CHROME_ELF_WHITELIST_WHITELIST_DEFERRAL_H_
