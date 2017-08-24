// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_SHELL_APP_PATHS_MAC_H_
#define CONTENT_SHELL_APP_PATHS_MAC_H_

namespace base {
class FilePath;
}

// Sets up base::mac::FrameworkBundle.
void OverrideFrameworkBundlePath();

// Sets up the CHILD_PROCESS_EXE path to properly point to the helper app.
void OverrideChildProcessPath();

// Sets up the DIR_SOURCE_ROOT path if this is a helper process.
void OverrideSourceRootPath();

// Gets the path to the content shell's pak file.
base::FilePath GetResourcesPakFilePath();

// Gets the path to content shell's Info.plist file.
base::FilePath GetInfoPlistPath();

#endif  // CONTENT_SHELL_APP_PATHS_MAC_H_
