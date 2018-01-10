// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/base_paths.h"

#include <stdlib.h>

#include "base/base_paths_fuchsia.h"
#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/process/process.h"

namespace base {
namespace {

constexpr char kPackageRoot[] = "/pkg";

}  // namespace

bool GetPackageRoot(base::FilePath* path) {
  *path = base::FilePath(kPackageRoot);
  if (PathExists(*path)) {
    return true;
  } else {
    *path = base::FilePath();
    return false;
  }
}

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
      if (!GetPackageRoot(result)) {
        *result = FilePath("/system");
      }
      return true;
    case DIR_CACHE:
      *result = FilePath("/data");
      return true;
    case DIR_FUCHSIA_RESOURCES:
      if (!GetPackageRoot(result)) {
        PathService::Get(DIR_EXE, result);
      }
      return true;
  }
  return false;
}

}  // namespace base
