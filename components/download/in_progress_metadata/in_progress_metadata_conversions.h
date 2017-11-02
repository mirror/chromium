// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DOWNLOAD_IN_PROGRESS_METADATA_IN_PROGRESS_METADATA_CONVERSIONS_H_
#define COMPONENTS_DOWNLOAD_IN_PROGRESS_METADATA_IN_PROGRESS_METADATA_CONVERSIONS_H_

#include "base/macros.h"
#include "components/download/in_progress_metadata/download_entry.h"
#include "components/download/in_progress_metadata/proto/download_entry.pb.h"

namespace active_downloads {

class InProgressMetadataConversions {
 public:
  static DownloadEntry DownloadEntryFromProto(
      const metadata_pb::DownloadEntry& proto);

  static metadata_pb::DownloadEntry DownloadEntryToProto(
      const DownloadEntry& entry);

  static std::vector<DownloadEntry> DownloadEntriesFromProto(
      const metadata_pb::DownloadEntries& proto);

  static metadata_pb::DownloadEntries DownloadEntriesToProto(
      const std::vector<DownloadEntry>& entries);

  DISALLOW_IMPLICIT_CONSTRUCTORS(InProgressMetadataConversions);
};

}  // namespace active_downloads

#endif  // COMPONENTS_DOWNLOAD_IN_PROGRESS_METADATA_IN_PROGRESS_METADATA_CONVERSIONS_H_
