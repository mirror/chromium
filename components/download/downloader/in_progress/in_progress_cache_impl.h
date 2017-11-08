// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DOWNLOAD_IN_PROGRESS_CACHE_IMPL_H_
#define COMPONENTS_DOWNLOAD_IN_PROGRESS_CACHE_IMPL_H_

#include <string>

#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/sequenced_task_runner.h"
#include "components/download/downloader/in_progress/download_entry.h"
#include "components/download/downloader/in_progress/in_progress_cache.h"
#include "components/download/downloader/in_progress/proto/download_entry.pb.h"

namespace download {

// InProgressCache provides a write-through cache that persists
// information related to an in-progress download such as request origin, retry
// count, resumption parameters etc to the disk. The entries are written to disk
// right away.
class InProgressCacheImpl : public InProgressCache {
 public:
  explicit InProgressCacheImpl(
      const base::FilePath& cache_file_path,
      const scoped_refptr<base::SequencedTaskRunner>& task_runner);
  ~InProgressCacheImpl() override;

  // InProgressCache implementation.
  void AddOrReplaceEntry(const DownloadEntry& entry) override;
  base::Optional<DownloadEntry> RetrieveEntry(const std::string& guid) override;
  void RemoveEntry(const std::string& guid) override;

 private:
  metadata_pb::DownloadEntries entries_;
  base::FilePath file_path_;
  scoped_refptr<base::SequencedTaskRunner> task_runner_;

  DISALLOW_COPY_AND_ASSIGN(InProgressCacheImpl);
};

}  // namespace download

#endif  // COMPONENTS_DOWNLOAD_IN_PROGRESS_CACHE_IMPL_H_
