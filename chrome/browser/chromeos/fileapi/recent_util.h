// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Utilities for Recent file system.

#ifndef CHROME_BROWSER_CHROMEOS_FILEAPI_RECENT_UTIL_H_
#define CHROME_BROWSER_CHROMEOS_FILEAPI_RECENT_UTIL_H_

#include <string>

#include "base/callback_forward.h"
#include "base/files/file_path.h"
#include "chrome/browser/chromeos/fileapi/recent_context.h"

class Profile;

namespace storage {

class FileSystemURL;

}  // namespace storage

namespace chromeos {

using ParseRecentUrlCallback =
    base::OnceCallback<void(RecentContext context, const base::FilePath& path)>;

// Returns the mount point path for a profile.
// This function must be called on the UI thread.
base::FilePath GetRecentMountPointPath(Profile* profile);

// Returns the mount point name for a profile.
// This function must be called on the UI thread.
std::string GetRecentMountPointName(Profile* profile);

// Parses a FileSystemURL of Recent file system.
// The callback is invoked with two arguments: |context| is a RecentContext
// to be used for operations, and |path| is a base file name in Recent file
// system.
// This function must be called on the IO thread, and the callback will be
// invoked on the IO thread.
void ParseRecentUrlOnIOThread(storage::FileSystemContext* file_system_context,
                              const storage::FileSystemURL& url,
                              ParseRecentUrlCallback callback);

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_FILEAPI_RECENT_UTIL_H_
