// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome_elf/whitelist/whitelist_component.h"

#include <assert.h>
#include <string.h>

//#include <algorithm>
//#include <memory>
//#include <vector>
//
//#include "chrome_elf/nt_registry/nt_registry.h"

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

}  // namespace

//------------------------------------------------------------------------------
// Public defines & functions
//------------------------------------------------------------------------------

// Grab the latest component whitelist.
// Keep debug asserts for unexpected behaviour contained in here.
ComponentStatus InitComponent() {
  // Debug check: InitIMEs should not be called more than once.

  return kComponentSuccess;
}

}  // namespace whitelist
