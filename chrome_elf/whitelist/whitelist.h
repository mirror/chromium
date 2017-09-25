// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_ELF_WHITELIST_WHITELIST_H_
#define CHROME_ELF_WHITELIST_WHITELIST_H_

#include <stddef.h>

namespace whitelist {

// Initializes the DLL whitelist in the current process. This should be called
// before any undesirable DLLs might be loaded.
bool Init();

}  // namespace whitelist

#endif  // CHROME_ELF_WHITELIST_WHITELIST_H_
