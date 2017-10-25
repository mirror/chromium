// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome_elf/whitelist/whitelist_deferral.h"

#include <assert.h>

//
//#include <algorithm>
//#include <memory>
//#include <vector>
#include "chrome/install_static/install_util.h"
#include "chrome_elf/nt_registry/nt_registry.h"
#include "chrome_elf/whitelist/whitelist.h"

namespace whitelist {
namespace {

//------------------------------------------------------------------------------
// Private functions
//------------------------------------------------------------------------------

// List of unknown images to be verified.
// NOTE: this list is only initialized once during InitDeferrals().
// NOTE: this structure is wrapped in a function to prevent an exit-time dtor.
std::vector<std::wstring>* GetDeferralVector() {
  static std::vector<std::wstring>* const deferral_list =
      new std::vector<std::wstring>();

  return deferral_list;
}

}  // namespace

//------------------------------------------------------------------------------
// Public defines & functions
//------------------------------------------------------------------------------

void AddDeferral(std::wstring&& path) {
  GetDeferralVector()->push_back(std::move(path));
}

size_t GetDeferralListSize() {
  return GetDeferralVector()->size();
}

// Initialize deferral infrastructure.
// Keep debug asserts for unexpected behaviour contained in here.
DeferralStatus InitDeferrals() {
  std::vector<std::wstring>* deferral_list = GetDeferralVector();

  // Debug check: InitDeferrals should not be called more than once.
  assert(deferral_list->empty());
  deferral_list->clear();

  return DeferralStatus::kSuccess;
}

}  // namespace whitelist
