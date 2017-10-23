// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome_elf/whitelist/whitelist_component.h"

#include <windows.h>

//#include "base/files/file.h"
//#include "base/test/test_reg_util_win.h"
//#include "base/win/registry.h"
//#include "chrome_elf/nt_registry/nt_registry.h"
//#include "sandbox/win/src/nt_internals.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace whitelist {
namespace {

//------------------------------------------------------------------------------
// Whitelist component tests
//------------------------------------------------------------------------------

// Test initialization with whatever IMEs are on the current machine.
TEST(Whitelist, ComponentInit) {
  // Init IME!
  EXPECT_EQ(InitComponent(), kComponentSuccess);
}

}  // namespace
}  // namespace whitelist
