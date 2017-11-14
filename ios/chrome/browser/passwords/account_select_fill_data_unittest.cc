// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/passwords/account_select_fill_data.h"

#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using password_manager::AccountSelectFillData;

namespace {

class AccountSelectFillDataTest : public testing::Test {};

TEST_F(AccountSelectFillDataTest, OneForm) {
  AccountSelectFillData account_select_fill_data;
  EXPECT_TRUE(account_select_fill_data.Empty());
}

}  // namespace
