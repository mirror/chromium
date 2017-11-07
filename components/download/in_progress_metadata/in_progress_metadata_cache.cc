// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/download/in_progress_metadata/in_progress_metadata_cache.h"

namespace download {

InProgressMetadataCache::InProgressMetadataCache(
    const base::FilePath& cache_file_path,
    const scoped_refptr<base::SequencedTaskRunner>& task_runner) {}

InProgressMetadataCache::~InProgressMetadataCache() = default;

void InProgressMetadataCache::AddOrUpdateMetadata(const DownloadEntry& entry) {
  // TODO(jming): Implementation.
}

DownloadEntry* InProgressMetadataCache::RetrieveMetadata(
    const std::string& guid) {
  // TODO(jming): Implementation.
  return nullptr;
}

void InProgressMetadataCache::RemoveMetadata(const std::string& guid) {
  // TODO(jming): Implementation.
}

}  // namespace download
