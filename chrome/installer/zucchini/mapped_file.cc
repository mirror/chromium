// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/zucchini/mapped_file.h"

#include <utility>

#include "base/files/file_util.h"
#include "base/logging.h"
#include "build/build_config.h"

namespace zucchini {

MappedFileReader::MappedFileReader(const base::FilePath& file_path) {
  bool is_ok = buffer_.Initialize(file_path);
  if (!is_ok)
    LOG(ERROR) << "Can't read file: " << file_path.value();
}

MappedFileWriter::MappedFileWriter(const base::FilePath& file_path,
                                   size_t length,
                                   bool should_delete)
    : file_path_(file_path), should_delete_(should_delete) {
  using base::File;
  File file(file_path_, File::FLAG_CREATE_ALWAYS | File::FLAG_READ |
                            File::FLAG_WRITE | File::FLAG_SHARE_DELETE |
                            File::FLAG_CAN_DELETE_ON_CLOSE);

  file_handle_ = file.Duplicate();
#if defined(OS_WIN)
  if (should_delete_) {
    // Tell the OS to delete the file when all handles are closed.
    will_delete_on_close_ = file_handle_.DeleteOnClose(true);
    LOG_IF(WARNING, !will_delete_on_close_)
        << "Failed marking file for delete-on-close: " << file_path_.value();
  }
#endif

  bool is_ok =
      buffer_.Initialize(std::move(file), {0, static_cast<int64_t>(length)},
                         base::MemoryMappedFile::READ_WRITE_EXTEND);
  if (!is_ok)
    LOG(ERROR) << "Can't create file: " << file_path_.value();
}

MappedFileWriter::~MappedFileWriter() {
  if (should_delete_ && !will_delete_on_close_ &&
      !base::DeleteFile(file_path_, false)) {
    LOG(WARNING) << "Failed to delete file: " << file_path_.value();
  }
}

bool MappedFileWriter::Keep() {
#if defined(OS_WIN)
  // Keep the file.
  if (will_delete_on_close_ && !file_handle_.DeleteOnClose(false)) {
    LOG(DFATAL) << "Failed preventing deletion of file: " << file_path_.value();
    return false;
  }
#endif

  return true;
}

}  // namespace zucchini
