// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_WEBRTC_WEBRTC_LOGS_DIRECTORY_ACCESS_UTIL_H_
#define CHROME_BROWSER_MEDIA_WEBRTC_WEBRTC_LOGS_DIRECTORY_ACCESS_UTIL_H_

#include <stdint.h>

class Profile;

namespace extensions {
struct GrantedFileEntry;
}

namespace webrtc_logs_directory_access_util {

extensions::GrantedFileEntry GetLogsDirectoryEntry(Profile* profile,
                                                   int render_process_id);

}  // namespace webrtc_logs_directory_access_util

#endif  // CHROME_BROWSER_MEDIA_WEBRTC_WEBRTC_LOGS_DIRECTORY_ACCESS_UTIL_H_
