// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_ZLIB_GOOGLE_ZIP_WRITER_H_
#define THIRD_PARTY_ZLIB_GOOGLE_ZIP_WRITER_H_

#include <memory>
#include <vector>

#include "base/files/file_path.h"
#include "build/build_config.h"
#include "third_party/zlib/google/zip.h"

#if defined(USE_SYSTEM_MINIZIP)
#include <minizip/unzip.h>
#include <minizip/zip.h>
#else
#include "third_party/zlib/contrib/minizip/unzip.h"
#include "third_party/zlib/contrib/minizip/zip.h"
#endif

namespace zip {
namespace internal {

// A class used to write entries to a ZIP file and buffering the writes to limit
// the number of calls to the FileAccessor. (for performance reasons as these
// calls may be expensive when IPC based).
// This class is so far internal and used by zip.cc.

class ZipWriter {
 public:
#if defined(OS_POSIX)
  static std::unique_ptr<ZipWriter> CreateWithFd(int zip_file_fd,
                                                 const base::FilePath& root_dir,
                                                 FileAccessor* file_accessor);
#endif
  static std::unique_ptr<ZipWriter> Create(const base::FilePath& zip_file,
                                           const base::FilePath& root_dir,
                                           FileAccessor* file_accessor);

  ~ZipWriter();

  bool AddEntries(const std::vector<base::FilePath>& paths);

  bool Close();

 private:
  ZipWriter(zipFile zip_file,
            const base::FilePath& root_dir,
            FileAccessor* file_accessor);

  bool FlushEntriesIfNeeded(bool force);

  std::vector<base::FilePath> pending_entries_;
  zipFile zip_file_;
  base::FilePath root_dir_;
  FileAccessor* file_accessor_;

  // Numbers of pending entries that trigger writting them to the ZIP file.S
  static size_t kMaxPendingEntriesCount;

  DISALLOW_COPY_AND_ASSIGN(ZipWriter);
};

}  // namespace internal
}  // namespace zip

#endif  // #define THIRD_PARTY_ZLIB_GOOGLE_ZIP_WRITER_H_