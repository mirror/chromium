// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/fileapi/test/fake_recent_source.h"

#include <utility>

#include "base/files/file.h"
#include "base/memory/ptr_util.h"
#include "chrome/browser/chromeos/fileapi/recent_context.h"
#include "storage/browser/fileapi/file_stream_reader.h"
#include "storage/browser/fileapi/file_system_operation.h"

namespace chromeos {

class FakeRecentSource::FakeRecentFile : public RecentFile {
 public:
  FakeRecentFile(SourceType source_type,
                 const std::string& original_file_name,
                 const std::string& unique_id,
                 int64_t size);
  ~FakeRecentFile() override;

  std::unique_ptr<FakeRecentFile> Clone() const;

  // RecentFile overrides:
  void GetFileInfo(int fields,
                   storage::FileSystemContext* file_system_context,
                   const GetFileInfoCallback& callback) override;
  std::unique_ptr<storage::FileStreamReader> CreateFileStreamReader(
      int64_t offset,
      int64_t max_bytes_to_read,
      const base::Time& expected_modification_time,
      storage::FileSystemContext* file_system_context) override;

 private:
  int64_t size_;

  DISALLOW_COPY_AND_ASSIGN(FakeRecentFile);
};

FakeRecentSource::FakeRecentFile::FakeRecentFile(
    SourceType source_type,
    const std::string& original_file_name,
    const std::string& unique_id,
    int64_t size)
    : RecentFile(source_type, original_file_name, unique_id), size_(size) {}

FakeRecentSource::FakeRecentFile::~FakeRecentFile() = default;

std::unique_ptr<FakeRecentSource::FakeRecentFile>
FakeRecentSource::FakeRecentFile::Clone() const {
  return base::MakeUnique<FakeRecentFile>(source_type(), original_file_name(),
                                          unique_id(), size_);
}

void FakeRecentSource::FakeRecentFile::GetFileInfo(
    int fields,
    storage::FileSystemContext* file_system_context,
    const GetFileInfoCallback& callback) {
  base::File::Info info;
  if ((fields & storage::FileSystemOperation::GET_METADATA_FIELD_SIZE) != 0)
    info.size = size_;
  if ((fields &
       storage::FileSystemOperation::GET_METADATA_FIELD_IS_DIRECTORY) != 0)
    info.is_directory = false;
  callback.Run(base::File::FILE_OK, info);
}

std::unique_ptr<storage::FileStreamReader>
FakeRecentSource::FakeRecentFile::CreateFileStreamReader(
    int64_t offset,
    int64_t max_bytes_to_read,
    const base::Time& expected_modification_time,
    storage::FileSystemContext* file_system_context) {
  NOTIMPLEMENTED();
  return nullptr;
}

FakeRecentSource::FakeRecentSource(RecentFile::SourceType source_type)
    : source_type_(source_type) {}

FakeRecentSource::~FakeRecentSource() = default;

void FakeRecentSource::AddFile(const std::string& original_file_name,
                               const std::string& unique_id,
                               int64_t size) {
  canned_files_.emplace_back(base::MakeUnique<FakeRecentFile>(
      source_type_, original_file_name, unique_id, size));
}

void FakeRecentSource::GetRecentFiles(RecentContext context,
                                      GetRecentFilesCallback callback) {
  std::vector<std::unique_ptr<RecentFile>> files;
  for (const auto& file : canned_files_)
    files.emplace_back(file->Clone());
  std::move(callback).Run(std::move(files));
}

}  // namespace chromeos
