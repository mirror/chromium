// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DOWNLOAD_IN_PROGRESS_METADATA_CACHE_H_
#define COMPONENTS_DOWNLOAD_IN_PROGRESS_METADATA_CACHE_H_

#include <string>

#include "base/files/file_path.h"
#include "base/macros.h"
#include "components/download/in_progress_metadata/download_entry.h"
#include "components/download/in_progress_metadata/proto/download_entry.pb.h"

namespace active_downloads {

// InProgressMetadataCache provides a write-through cache that persists
// information related to an in-progress download such as request origin, retry
// count, resumption parameters etc to the disk. The entries are written to disk
// right away.
class InProgressMetadataCache {
 public:
  InProgressMetadataCache(const base::FilePath& file_path);
  ~InProgressMetadataCache();

  // Adds a new entry or updates an existing entry.
  void AddOrUpdateMetadata(const DownloadEntry& entry);

  // Retrieves an existing entry.
  DownloadEntry* RetrieveMetadata(const std::string& guid);

  // Remove an entry.
  void RemoveMetadata(const std::string& guid);

 private:
  base::FilePath file_path_;
  metadata_pb::DownloadEntries entries_;
  const scoped_refptr<base::SequencedTaskRunner> task_runner_;

  DISALLOW_COPY_AND_ASSIGN(InProgressMetadataCache);
};

}  // namespace active_downloads

#endif  // COMPONENTS_DOWNLOAD_IN_PROGRESS_METADATA_CACHE_H_
