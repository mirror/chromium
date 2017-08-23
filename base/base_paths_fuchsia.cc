// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/base_paths.h"

#include <stdlib.h>

#include "base/command_line.h"
#include "base/files/file_path.h"

namespace base {

bool PathProviderFuchsia(int key, FilePath* result) {
  // TODO(fuchsia): There's no API to retrieve these on Fuchsia. The app name
  // itself should be dynamic (i.e. not always "chrome") but other paths are
  // correct as fixed paths like this. See https://crbug.com/726124.
  switch (key) {
    case FILE_EXE:
    case FILE_MODULE:
      // Use the binary name as specified on the command line.
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
    case DIR_SOURCE_ROOT:
      // This is only used for tests, so we return the binary location for now.
      *result = FilePath("/system");
      return true;
    case DIR_CACHE:
      *result = FilePath("/data");
      return true;
  }

  return false;
}

}  // namespace base
