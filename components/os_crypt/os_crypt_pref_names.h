// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OS_CRYPT_OS_CRYPT_PREF_NAMES_H_
#define COMPONENTS_OS_CRYPT_OS_CRYPT_PREF_NAMES_H_
#endif  // COMPONENTS_OS_CRYPT_OS_CRYPT_PREF_NAMES_H_

namespace os_crypt {
namespace prefs {

// #if defined(OS_LINUX) && !defined(OS_CHROMEOS)
// The value of this preference controls whether OSCrypt on Linux will try to
// store and read the encryption key from a backend (Keyring, KWallet etc).
extern const char kDisableEncryption[];
// #endif

}  // namespace prefs
}  // namespace os_crypt
