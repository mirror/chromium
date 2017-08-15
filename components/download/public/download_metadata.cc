// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/download/public/download_metadata.h"

namespace download {

DownloadMetaData::DownloadMetaData() : completed(false) {}

DownloadMetaData::DownloadMetaData(const DownloadMetaData& other) = default;

bool DownloadMetaData::operator==(const DownloadMetaData& other) const {
  return guid == other.guid && completed == other.completed &&
         path == other.path;
}

DownloadMetaData::~DownloadMetaData() = default;

}  // namespace download
