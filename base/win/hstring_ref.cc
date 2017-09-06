// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/win/hstring_ref.h"

#include "base/logging.h"
#include "base/win/com_base_util.h"

namespace base {
namespace win {

HStringRef::HStringRef(const base::char16* str) {
  HRESULT hr = ComBaseUtil::WindowsCreateStringReference(
      str, static_cast<uint32_t>(wcslen(str)), &header_, &hstr_);
  if (FAILED(hr))
    VLOG(1) << "WindowsCreateStringReference failed";
}

HSTRING HStringRef::Get() {
  return hstr_;
}

}  // namespace win
}  // namespace base
