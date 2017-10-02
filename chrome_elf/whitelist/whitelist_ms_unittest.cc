// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome_elf/whitelist/whitelist_ms.h"

#include <windows.h>

#include "sandbox/win/src/nt_internals.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

//------------------------------------------------------------------------------
// Whitelist MS tests
//------------------------------------------------------------------------------

// Test
TEST(Whitelist, Microsoft) {
  // Get a path to a system dll to test.

  // Init IME!
  // whitelist::IMEStatus status = whitelist::InitIMEs();
  // EXPECT_EQ(status, whitelist::WL_SUCCESS);
}

}  // namespace
