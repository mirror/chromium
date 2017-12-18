// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/download/downloader/in_progress/download_entry.h"

namespace download {

DownloadEntry::DownloadEntry() = default;

DownloadEntry::DownloadEntry(const DownloadEntry& other) = default;

DownloadEntry::DownloadEntry(const std::string& guid,
                             int ukm_download_id,
                             ukm::SourceId ukm_source_id)
    : guid(guid),
      ukm_download_id(ukm_download_id),
      ukm_source_id(ukm_source_id) {}

DownloadEntry::DownloadEntry(const std::string& guid,
                             const std::string& request_origin,
                             DownloadSource download_source,
                             int ukm_download_id)
    : guid(guid),
      request_origin(request_origin),
      download_source(download_source),
      ukm_download_id(ukm_download_id) {}

DownloadEntry::~DownloadEntry() = default;

bool DownloadEntry::operator==(const DownloadEntry& other) const {
  return guid == other.guid && request_origin == other.request_origin &&
         download_source == other.download_source &&
         ukm_download_id == other.ukm_download_id &&
         ukm_source_id == other.ukm_source_id;
}

bool DownloadEntry::operator!=(const DownloadEntry& other) const {
  return !(*this == other);
}

}  // namespace download
