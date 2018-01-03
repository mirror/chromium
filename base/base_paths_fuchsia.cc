// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/base_paths.h"

#include <stdlib.h>

#include "base/command_line.h"
#include "base/files/file_path.h"

namespace base {
namespace {

const char kPackageBinaryLocation[] = "/pkg/bin/app";
const char kPackageUrlPrefix[] = "file://";

// TODO(fuchsia): We might need to have a more robust solution for detecting the
// package context in case we launch separate executables, or eliminate the
// option of running outside a package entirely.
bool IsPackageProcess() {
  std::string exe_path =
      base::CommandLine::ForCurrentProcess()->GetProgram().AsUTF8Unsafe();
  return exe_path.find(kPackageUrlPrefix) == 0 ||
         exe_path == kPackageBinaryLocation;
}

bool GetExecutablePath(FilePath* result) {
  // The path to the binary executable inside a packaged app is always
  // "/pkg/bin/app", but the path indicated by argv is a URL which looks more
  // like this: file://system/foo.far. The URL-based executable path is
  // meaningless within the context of the process, so we must detect that case
  // and convert it to the actual executable path in the process' namespace.
  if (IsPackageProcess()) {
    *result = FilePath(kPackageBinaryLocation);
    return true;
  }

  char bin_dir[PATH_MAX + 1];
  if (realpath(base::CommandLine::ForCurrentProcess()
                   ->GetProgram()
                   .AsUTF8Unsafe()
                   .c_str(),
               bin_dir) == NULL) {
    return false;
  }

  *result = FilePath(bin_dir);
  return true;
}

}  // namespace

bool PathProviderFuchsia(int key, FilePath* result) {
  if (IsPackageProcess()) {
    // We use an alternate set of path mappings depending on whether or not
    // the process is executed from within a package.
    // Packaged processes have a substantially different namespace layout than
    // regular processes (ones invoked outside the application controller).
    switch (key) {
      case FILE_MODULE:
      // Not supported in debug or component builds. Fall back on using the EXE
      // path for now.
      // TODO(fuchsia): Get this value from an API. See https://crbug.com/726124
      case FILE_EXE:
        return GetExecutablePath(result);
      case DIR_SOURCE_ROOT:
        *result = FilePath("/pkg");
        return true;
      case DIR_CACHE:
        *result = FilePath("/data");
        return true;
      case DIR_EXE:
        *result = FilePath("/pkg/lib");
        return true;
    }

    return false;
  } else {
    switch (key) {
      case FILE_MODULE:
      // Not supported in debug or component builds. Fall back on using the EXE
      // path for now.
      // TODO(fuchsia): Get this value from an API. See https://crbug.com/726124
      case FILE_EXE:
        return GetExecutablePath(result);
      case DIR_SOURCE_ROOT:
        // This is only used for tests, so we return the binary location for
        // now.
        *result = FilePath("/system");
        return true;
      case DIR_CACHE:
        *result = FilePath("/data");
        return true;
    }
    return false;
  }
}

}  // namespace base
