// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/base_paths.h"

#include <stdlib.h>

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"

namespace base {
namespace {

const char kPackageRoot[] = "/pkg";

bool IsPackageProcess() {
  return PathExists(base::FilePath(kPackageRoot));
}

}  // namespace

bool PathProviderFuchsia(int key, FilePath* result) {
  switch (key) {
    case FILE_MODULE:
      NOTIMPLEMENTED();
      return false;
    case FILE_EXE: {
      *result = base::MakeAbsoluteFilePath(base::FilePath(
          base::CommandLine::ForCurrentProcess()->GetProgram().AsUTF8Unsafe()));
      return true;
    }
    case DIR_SOURCE_ROOT:
      if (IsPackageProcess()) {
        *result = FilePath(kPackageRoot);
      } else {
        *result = FilePath("/system");
      }
      return true;
    case DIR_CACHE:
      *result = FilePath("/data");
      return true;
    case DIR_EXE:
      if (IsPackageProcess()) {
        *result = FilePath(kPackageRoot);
        return true;
      }
      return false;
  }
  return false;
}

}  // namespace base
