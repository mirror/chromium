// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_ELF_WHITELIST_WHITELIST_IME_H_
#define CHROME_ELF_WHITELIST_WHITELIST_IME_H_

namespace whitelist {

enum IMEStatus {
  kImeSuccess = 0,
  kImeErrorGeneric = 1,
  kImeMissingRegKey = 2,
  kImeRegAccessDenied = 3,
  kImeGenericRegFailure = 4
};

// Constant registry locations for IME information.
extern const wchar_t kClassesKey[];
extern const wchar_t kClassesSubkey[];
extern const wchar_t kImeRegistryKey[];

// Initialize internal list of registered IMEs.
IMEStatus InitIMEs();

}  // namespace whitelist

#endif  // CHROME_ELF_WHITELIST_WHITELIST_IME_H_
