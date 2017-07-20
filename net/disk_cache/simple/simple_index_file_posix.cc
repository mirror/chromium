// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/disk_cache/simple/simple_index_file.h"

#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <memory>
#include <string>

#include "base/files/file_util.h"
#include "base/logging.h"

namespace disk_cache {
namespace {

struct DirCloser {
  void operator()(DIR* dir) { closedir(dir); }
};

typedef std::unique_ptr<DIR, DirCloser> ScopedDir;

}  // namespace

// static
bool SimpleIndexFile::TraverseCacheDirectory(
    const base::FilePath& cache_path,
    const EntryFileCallback& entry_file_callback) {
  const ScopedDir dir(opendir(cache_path.value().c_str()));
  if (!dir) {
    PLOG(ERROR) << "opendir " << cache_path.value();
    return false;
  }
  dirent* entry;
  // errno must be set 0 before every readdir() call to detect errors.
  while ((entry = (errno = 0, readdir(dir.get()))) != nullptr) {
    const std::string file_name(entry->d_name);
    if (file_name == "." || file_name == "..")
      continue;
    const base::FilePath file_path = cache_path.Append(
        base::FilePath(file_name));
    base::File::Info file_info;
    if (!base::GetFileInfo(file_path, &file_info)) {
      LOG(ERROR) << "Could not get file info for " << file_path.value();
      continue;
    }

    entry_file_callback.Run(file_path, file_info.last_accessed,
                            file_info.last_modified, file_info.size);
  }

  if (errno) {
    PLOG(ERROR) << "readdir " << cache_path.value();
    return false;
  }

  return true;
}

}  // namespace disk_cache
