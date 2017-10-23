// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/download/in_progress_metadata/in_progress_metadata_conversions.h"

#include <utility>

namespace active_downloads {

DownloadEntry InProgressMetadataConversions::DownloadEntryFromProto(
    const metadata_pb::DownloadEntry& proto) {
  DownloadEntry entry;
  entry.guid = proto.guid();
  entry.request_origin = proto.request_origin();
  return entry;
}

metadata_pb::DownloadEntry InProgressMetadataConversions::DownloadEntryToProto(
    const DownloadEntry& entry) {
  metadata_pb::DownloadEntry proto;
  proto.set_guid(entry.guid);
  proto.set_request_origin(entry.request_origin);
  return proto;
}

}  // namespace active_downloads
