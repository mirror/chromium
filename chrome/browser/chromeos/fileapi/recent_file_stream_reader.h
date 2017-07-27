// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_FILEAPI_RECENT_FILE_STREAM_READER_H_
#define CHROME_BROWSER_CHROMEOS_FILEAPI_RECENT_FILE_STREAM_READER_H_

#include <memory>
#include <vector>

#include "base/callback_forward.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/chromeos/fileapi/recent_model.h"
#include "storage/browser/fileapi/file_stream_reader.h"

namespace base {

class Time;

}  // namespace base

namespace storage {

class FileSystemContext;
class FileSystemURL;

}  // namespace storage

namespace chromeos {

// FileStreamReader implementation for recent file system.
// It actually delegates operations to other FileStreamReader.
// TODO(nya): Write unit tests.
class RecentFileStreamReader : public storage::FileStreamReader {
 public:
  RecentFileStreamReader(const storage::FileSystemURL& recent_url,
                         int64_t offset,
                         int64_t max_bytes_to_read,
                         const base::Time& expected_modification_time,
                         storage::FileSystemContext* file_system_context,
                         RecentModel* model);
  ~RecentFileStreamReader() override;

  // storage::FileStreamReader override:
  int Read(net::IOBuffer* buffer,
           int buffer_length,
           const net::CompletionCallback& callback) override;
  int64_t GetLength(const net::Int64CompletionCallback& callback) override;

 private:
  void OnParseRecentUrl(
      scoped_refptr<storage::FileSystemContext> file_system_context,
      RecentModel* model,
      RecentContext context,
      const base::FilePath& path);
  void OnGetFilesMap(
      scoped_refptr<storage::FileSystemContext> file_system_context,
      const base::FilePath& path,
      const RecentModel::FilesMap& files_map);
  void OnReaderResolved(
      std::unique_ptr<storage::FileStreamReader> underlying_reader);

  void RunPendingRead(scoped_refptr<net::IOBuffer> buffer,
                      int buffer_length,
                      const net::CompletionCallback& callback);
  void RunPendingGetLength(const net::Int64CompletionCallback& callback);

  int64_t offset_;
  int64_t max_bytes_to_read_;
  base::Time expected_modification_time_;

  bool reader_resolved_;
  std::unique_ptr<storage::FileStreamReader> underlying_reader_;
  std::vector<base::OnceClosure> pending_operations_;

  base::WeakPtrFactory<RecentFileStreamReader> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(RecentFileStreamReader);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_FILEAPI_RECENT_FILE_STREAM_READER_H_
