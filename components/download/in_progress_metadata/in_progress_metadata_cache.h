// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DOWNLOAD_IN_PROGRESS_METADATA_CACHE_H_
#define COMPONENTS_DOWNLOAD_IN_PROGRESS_METADATA_CACHE_H_

#include <map>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/macros.h"
#include "components/download/in_progress_metadata/proto/download_entry.pb.h"

namespace download {

// InProgressMetadataCache provides a cache that persists information related
// to an in-progress download such as request origin, retry count, resumption
// parameters etc to the disk. The data is persisted across restarts and is
// cleaned up after the download is completed.
class InProgressMetadataCache {
 public:
  InProgressMetadataCache(const base::FilePath& file_path);
  ~InProgressMetadataCache();

  void UpdateMetadata(metadata_pb::DownloadEntry* entry);
  metadata_pb::DownloadEntry* RetrieveMetadata(const std::string& guid);

 private:
  base::FilePath file_path_;
  metadata_pb::DownloadEntries entries_;

  DISALLOW_COPY_AND_ASSIGN(InProgressMetadataCache);
};

}  // namespace download

#endif  // COMPONENTS_DOWNLOAD_IN_PROGRESS_METADATA_CACHE_H_
