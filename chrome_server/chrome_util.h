// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_SERVER_CHROME_UTIL_H_
#define CHROME_SERVER_CHROME_UTIL_H_

#include "base/strings/string16.h"

namespace base {
class FilePath;
}

namespace chrome_server {

// Finalizes a previously updated installation.
void UpdateChromeIfNeeded(const base::FilePath& chrome_exe);

}  // namespace chrome_server

#endif  // CHROME_SERVER_CHROME_UTIL_H_
