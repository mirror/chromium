// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/download/in_progress_metadata/in_progress_metadata_cache.h"

#include "base/files/important_file_writer.h"

namespace download {

InProgressMetadataCache::InProgressMetadataCache(
    const base::FilePath& file_path) {}

InProgressMetadataCache::~InProgressMetadataCache() = default;

void InProgressMetadataCache::UpdateMetadata(
    metadata_pb::DownloadEntry* entry) {
  // TODO : Put |entry| into entries_ and write to file.
}

metadata_pb::DownloadEntry* InProgressMetadataCache::RetrieveMetadata(
    const std::string& guid) {
  // TODO: Read from file, filter entry and return.
  return nullptr;
}

}  // namespace download
