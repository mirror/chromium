// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_ELF_WHITELIST_WHITELIST_CACHE_H_
#define CHROME_ELF_WHITELIST_WHITELIST_CACHE_H_

namespace whitelist {

enum CacheStatus {
  kCacheSuccess = 0,
  kCacheRegPathFail,
  kCacheRegQueryValueFail,
};

// The key value name that holds the cache blob.
// Note: this value is in the kRegWhitelistTopKeyName key.
const wchar_t kRegWhitelistCacheValueName[];

// Initialize internal list of registered IMEs.
CacheStatus InitCache();

}  // namespace whitelist

#endif  // CHROME_ELF_WHITELIST_WHITELIST_CACHE_H_
