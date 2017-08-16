// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/download/public/download_metadata.h"

namespace download {

CompletionInfo::CompletionInfo() = default;

CompletionInfo::~CompletionInfo() = default;

bool CompletionInfo::operator==(const CompletionInfo& other) const {
  return path == other.path;
}

DownloadMetaData::DownloadMetaData() = default;

DownloadMetaData::DownloadMetaData(const DownloadMetaData& other) = default;

bool DownloadMetaData::operator==(const DownloadMetaData& other) const {
  return guid == other.guid && completion_info == other.completion_info;
}

DownloadMetaData::~DownloadMetaData() = default;

}  // namespace download
