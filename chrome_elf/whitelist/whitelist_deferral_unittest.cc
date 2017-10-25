// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome_elf/whitelist/whitelist_deferral.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace whitelist {
namespace {

//------------------------------------------------------------------------------
// Whitelist deferral tests
//------------------------------------------------------------------------------

// Test initialization with whatever IMEs are on the current machine.
TEST(Whitelist, DeferralInit) {
  // Init Deferrals!
  EXPECT_EQ(InitDeferrals(), DeferralStatus::kSuccess);
}

TEST(Whitelist, DeferralAdd) {
  EXPECT_EQ(GetDeferralListSize(), size_t{0});
  std::wstring str1(L"");
  AddDeferral(std::move(str1));
  std::wstring str2(L"some_path");
  AddDeferral(std::move(str2));
  EXPECT_EQ(GetDeferralListSize(), size_t{2});
}

}  // namespace
}  // namespace whitelist
