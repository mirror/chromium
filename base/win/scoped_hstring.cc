// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/win/scoped_hstring.h"

#include "base/logging.h"
#include "base/win/com_base_util.h"

namespace base {
namespace win {

// static
HSTRING ScopedHStringTraits::InvalidValue() {
  return nullptr;
}

// static
void ScopedHStringTraits::Free(HSTRING hstr) {
  ComBaseUtil::WindowsDeleteString(hstr);
}

ScopedHString::ScopedHString(const base::char16* str) : ScopedGeneric(nullptr) {
  HSTRING hstr;
  HRESULT hr = ComBaseUtil::WindowsCreateString(
      str, static_cast<uint32_t>(wcslen(str)), &hstr);
  if (FAILED(hr))
    VLOG(1) << "WindowsCreateString failed";
  else
    reset(hstr);
}

}  // namespace win
}  // namespace base
