// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_FILEAPI_RECENT_FILE_H_
#define CHROME_BROWSER_CHROMEOS_FILEAPI_RECENT_FILE_H_

#include <stdint.h>

#include <memory>
#include <string>

#include "base/files/file.h"
#include "base/macros.h"
#include "base/time/time.h"
#include "storage/browser/fileapi/async_file_util.h"

namespace storage {

class FileSystemContext;
class FileStreamReader;

}  // namespace storage

namespace chromeos {

// Abstract base class for a file entry in Recent file system.
class RecentFile {
 public:
  using GetFileInfoCallback = storage::AsyncFileUtil::GetFileInfoCallback;
  enum class SourceType {
    // Following types are used only in unit tests.
    TEST_1,
    TEST_2,
  };

  virtual ~RecentFile();

  // Returns a FileStreamReader for this file.
  // See FileSystemBackend::CreateFileStreamReader() for details.
  virtual std::unique_ptr<storage::FileStreamReader> CreateFileStreamReader(
      int64_t offset,
      int64_t max_bytes_to_read,
      const base::Time& expected_modification_time,
      storage::FileSystemContext* file_system_context) = 0;

  // Returns the source type of this file.
  SourceType source_type() const { return source_type_; }

  // Returns the file name in the source file system.
  // This name must conform to Chrome OS file name restrictions, e.g. must not
  // contain slashes.
  // Note that this might not be a file name appearing in Recent file system
  // due to file name conflicts. In such cases, this will differ from the
  // corresponding base::FilePath key in RecentModel::FilesMap.
  const std::string& file_name() const { return file_name_; }

  // Returns the unique ID of this file.
  // This value should be unique within the source file system. This value will
  // be used to track files across multiple queries to sources.
  const std::string& unique_id() const { return unique_id_; }

  // Metadata of this file.
  const base::File::Info& file_info() const { return file_info_; }

 protected:
  RecentFile(SourceType source_type,
             const std::string& file_name,
             const std::string& unique_id,
             const base::File::Info& file_info);

 private:
  const SourceType source_type_;
  const std::string file_name_;
  const std::string unique_id_;
  const base::File::Info file_info_;

  DISALLOW_COPY_AND_ASSIGN(RecentFile);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_FILEAPI_RECENT_FILE_H_
