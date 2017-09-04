// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/os_crypt/os_crypt_pref_names.h"

namespace os_crypt {
namespace prefs {

// #if defined(OS_LINUX) && !defined(OS_CHROMEOS)
const char kDisableEncryption[] = "os_crypt.disable_encryption";
// #endif

}  // namespace prefs
}  // namespace os_crypt
