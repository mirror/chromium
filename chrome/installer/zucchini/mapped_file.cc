// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/zucchini/mapped_file.h"

#include <utility>

#include "base/files/file_util.h"
#include "base/logging.h"
#include "build/build_config.h"

namespace zucchini {

MappedFileReader::MappedFileReader(base::File&& file) {
  if (!file.IsValid()) {
    status_ = "Invalid file.";
    return;  // |buffer_| will be uninitialized, and therefore invalid.
  }
  bool is_ok = buffer_.Initialize(std::move(file));
  if (!is_ok) {
    status_ = "Can't map file to memory.";
  }
}

MappedFileWriter::MappedFileWriter(const base::FilePath& file_path,
                                   base::File&& file,
                                   size_t length)
    : file_path_(file_path), delete_behavior_(kManualDeleteOnClose) {
  if (!file.IsValid()) {
    status_ = "Invalid file.";
    return;  // |buffer_| will be uninitialized, and therefore invalid.
  }

#if defined(OS_WIN)
  file_handle_ = file_.Duplicate();
  // Tell the OS to delete the file when all handles are closed.
  if (file_handle_.DeleteOnClose(true)) {
    delete_behavior_ = kAutoDeleteOnClose;
  } else {
    // TODO(ckitagawa): Get filename from somewhere.
    status_ = "Failed to mark file for delete-on-close.";
  }
#endif  // defined(OS_WIN)

  bool is_ok = buffer_.Initialize(std::move(file), {0, length},
                                  base::MemoryMappedFile::READ_WRITE_EXTEND);
  if (!is_ok) {
    status_ = "Can't map file to memory.";
  }
}

MappedFileWriter::~MappedFileWriter() {
  if (IsValid() && delete_behavior_ == kManualDeleteOnClose &&
      file_path_.value() != "" && !base::DeleteFile(file_path_, false)) {
    status_ = "Failed to delete file.";
  }
}

bool MappedFileWriter::Keep() {
#if defined(OS_WIN)
  if (delete_behavior_ == kAutoDeleteOnClose &&
      !file_handle_.DeleteOnClose(false)) {
    status_ = "Failed to prevent deletion of file.";
    return false;
  }
#endif  // defined(OS_WIN)
  delete_behavior_ = kKeep;
  return true;
}

}  // namespace zucchini
