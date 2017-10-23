// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome_elf/whitelist/whitelist_cache.h"

#include <assert.h>
#include <string.h>

//#include <algorithm>
//#include <memory>
//#include <vector>

#include "chrome/install_static/install_util.h"
#include "chrome_elf/nt_registry/nt_registry.h"
#include "chrome_elf/whitelist/whitelist.h"

namespace whitelist {
namespace {

//// Holds third-party IME information.
// class IME {
// public:
//  // Move constructor
//  IME(IME&&) noexcept = default;
//  // Move assignment
//  IME& operator=(IME&&) noexcept = default;
//  // Take ownership of the std::wstrings passed in, to prevent copies.
//  IME(std::wstring&& guid,
//      DWORD image_size,
//      DWORD date_time_stamp,
//      std::wstring&& path)
//      : image_size_(image_size),
//        date_time_stamp_(date_time_stamp),
//        guid_(std::move(guid)),
//        path_(std::move(path)) {}
//
//  DWORD image_size() { return image_size_; }
//  DWORD date_time_stamp() { return date_time_stamp_; }
//  const wchar_t* guid() { return guid_.c_str(); }
//  const wchar_t* path() { return path_.c_str(); }
//
// private:
//  DWORD image_size_;
//  DWORD date_time_stamp_;
//  std::wstring guid_;
//  std::wstring path_;
//
//  // DISALLOW_COPY_AND_ASSIGN(IME);
//  IME(const IME&) = delete;
//  IME& operator=(const IME&) = delete;
//};

//------------------------------------------------------------------------------
// Private functions
//------------------------------------------------------------------------------

CacheStatus GetRegCache() {
  // The install reg key should exist.
  std::wstring reg_path = install_static::GetRegistryPath();
  // The top-level whitelist key may or may not exist.
  reg_path = reg_path.append(kRegWhitelistTopKeyName);

  HANDLE key_handle = NULL;
  NTSTATUS status = STATUS_UNSUCCESSFUL;
  if (!nt::OpenRegKey(nt::HKCU, reg_path.c_str(), KEY_READ, &key_handle,
                      &status)) {
    if (status == STATUS_OBJECT_NAME_NOT_FOUND) {
      // There is no whitelist cache set up yet.  Init empty list.
      return kCacheSuccess;
    }
    return kCacheRegPathFail;
  }

  // Query the cache key value.
  ULONG type = 0;
  std::vector<BYTE> buffer;
  if (!nt::QueryRegKeyValue(key_handle, kRegWhitelistCacheValueName, &type,
                            &buffer) ||
      type != REG_BINARY) {
    return kCacheRegQueryValueFail;
  }

  // Got the cache blob.  Maybe it's a packed array like the component contents?

  return kCacheSuccess;
}

}  // namespace

//------------------------------------------------------------------------------
// Public defines & functions
//------------------------------------------------------------------------------

const wchar_t kRegWhitelistCacheValueName[] = L"cache";

// Harvest the registered IMEs in the system.
// Keep debug asserts for unexpected behaviour contained in here.
CacheStatus InitCache() {
  // Debug check: InitIMEs should not be called more than once.

  return kCacheSuccess;
}

}  // namespace whitelist
