// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_SERVER_CHROME_UTIL_H_
#define CHROME_SERVER_CHROME_UTIL_H_

namespace chrome_server {

// Launches chrome.exe if it is not running.
int LaunchChromeIfNotRunning();

}  // namespace chrome_server

#endif  // CHROME_SERVER_CHROME_UTIL_H_
