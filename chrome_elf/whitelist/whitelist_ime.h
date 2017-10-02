// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_ELF_WHITELIST_WHITELIST_IME_H_
#define CHROME_ELF_WHITELIST_WHITELIST_IME_H_

namespace whitelist {

enum IMEStatus : int {
  WL_SUCCESS = 0,
  WL_ERROR_GENERIC = 1,
  WL_MISSING_REG_KEY = 2,
  WL_REG_ACCESS_DENIED = 3,
  WL_GENERIC_REG_FAILURE = 4
};

// Constant registry locations for IME information.
constexpr wchar_t kClassesKey[] = L"SOFTWARE\\Classes\\CLSID\\";
constexpr wchar_t kClassesSubkey[] = L"InProcServer32";
constexpr wchar_t kImeRegistryKey[] = L"SOFTWARE\\Microsoft\\CTF\\TIP";

// Initialize internal list of registered IMEs.
IMEStatus InitIMEs();

}  // namespace whitelist

#endif  // CHROME_ELF_WHITELIST_WHITELIST_IME_H_
