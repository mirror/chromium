// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome_elf/whitelist/whitelist_cache.h"

#include <windows.h>

//#include "base/files/file.h"
#include "base/test/test_reg_util_win.h"
//#include "base/win/registry.h"
#include "chrome_elf/nt_registry/nt_registry.h"
//#include "sandbox/win/src/nt_internals.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace whitelist {
namespace {

void RegRedirect(nt::ROOT_KEY key,
                 registry_util::RegistryOverrideManager* rom) {
  ASSERT_NE(key, nt::AUTO);
  HKEY root = (key == nt::HKCU ? HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE);
  base::string16 temp;

  ASSERT_NO_FATAL_FAILURE(rom->OverrideRegistry(root, &temp));
  ASSERT_TRUE(nt::SetTestingOverride(key, temp));
}

void CancelRegRedirect(nt::ROOT_KEY key) {
  ASSERT_NE(key, nt::AUTO);
  ASSERT_TRUE(nt::SetTestingOverride(key, base::string16()));
}

//------------------------------------------------------------------------------
// Whitelist cache tests
//------------------------------------------------------------------------------

// Test initialization with a custom IME to exercise all of the code path for
// non-Microsoft-provided IMEs.
//
// Note: Uses registry test override to protect machine from registry writes.
TEST(Whitelist, Cache) {
  //// 1. Read in any real registry info needed.
  //// 2. Enable reg override for test net.
  //// 3. Write fake registry data.
  //// 4. InitIMEs()
  //// 5. Disable reg override.

  //// 1. Read in any real registry info needed.
  ////    - A dll path to use from an existing MS IME.
  // std::wstring existing_ime_dll;
  // ASSERT_TRUE(GetExistingImeDllPath(&existing_ime_dll));

  //// 2. Enable reg override for test net.
  // registry_util::RegistryOverrideManager override_manager;
  // ASSERT_NO_FATAL_FAILURE(RegRedirect(nt::HKLM, &override_manager));

  //// 3. Add a fake registry IME guid and Class DLL path.
  // EXPECT_TRUE(RegisterFakeIme(kNewGuid));
  // EXPECT_TRUE(AddClassDllPath(kNewGuid, existing_ime_dll));

  //// 4. Init IME!
  // EXPECT_EQ(InitIMEs(), kImeSuccess);

  //// 5. Disable reg override.
  // ASSERT_NO_FATAL_FAILURE(CancelRegRedirect(nt::HKLM));
}

}  // namespace
}  // namespace whitelist
