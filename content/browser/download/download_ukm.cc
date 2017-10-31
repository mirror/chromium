// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/download/download_ukm.h"

#include "services/metrics/public/cpp/ukm_builders.h"

namespace download_ukm {

DownloadUkmHelper::DownloadUkmHelper(ukm::UkmRecorder* ukm_recorder,
                                     const GURL& url)
    : ukm_recorder_(ukm_recorder), url_(url) {
  GetNewSourceId();
}

DownloadUkmHelper::~DownloadUkmHelper() = default;

// Functions that actually record the information.

void DownloadUkmHelper::RecordDownloadStarted(int file_type,
                                              int num_streams,
                                              int file_size,
                                              int component) {
  if (!CanLog())
    return;

  ukm::builders::Download_Started(source_id_)
      .SetFileType(file_type)
      .SetNumStreams(num_streams)
      .SetStartFileSize(file_size)
      .SetComponent(component)
      .Record(ukm_recorder_);
}

void DownloadUkmHelper::RecordDownloadStartedFileType(int file_type) {
  if (!CanLog())
    return;

  ukm::builders::Download_Started(source_id_)
      .SetFileType(file_type)
      .Record(ukm_recorder_);
}

// Utility functions to check state of UKM set-up.

void DownloadUkmHelper::GetNewSourceId() {
  ukm::SourceId source_id_ = ukm_recorder_->GetNewSourceID();
  ukm_recorder_->UpdateSourceURL(source_id_, url_);
}

bool DownloadUkmHelper::CanLog() {
  return ukm_recorder_ && url_.is_valid() && source_id_;
}

}  // namespace download_ukm
