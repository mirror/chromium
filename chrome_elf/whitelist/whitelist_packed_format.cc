// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "whitelist_packed_format.h"

namespace whitelist {

// Subdir relative to install_static::GetUserDataDirectory().
constexpr wchar_t kFileSubdir[] =
    L"\\ThirdPartyModuleList"
#if defined(_WIN64)
    L"64";
#else
    L"32";
#endif

// Packed module data cache file.
constexpr wchar_t kBlFileName[] = L"\\bldata";

}  // namespace whitelist
