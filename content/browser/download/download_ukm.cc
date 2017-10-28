// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/download/download_ukm.h"

DownloadUkmHelper::DownloadUkmHelper(ukm::UkmRecorder* ukm_recorder)
    : ukm_recorder_(ukm_recorder) {}

// Functions that actually record the information.

void DownloadUkmHelper::RecordDownloadStarted(ukm::SourceId source_id,
                                              int file_type,
                                              int num_streams,
                                              int file_size,
                                              int component) {
  if (!CanLog() || source_id == -1)
    return;

  ukm::builders::Download_Started(source_id)
      .SetFileType(file_type)
      .SetNumStreams(num_streams)
      .SetFileSize(file_size)
      .SetComponent(component)
      .Record(ukm_recorder_);
}

void DownloadUkmHelper::RecordDownloadStartedFileType(ukm::SourceId source_id,
                                                      int file_type) {
  if (!CanLog() || source_id == -1)
    return;

  ukm::builders::Download_Started(source_id).SetFileType(file_type).Record(
      ukm_recorder_);
}

// Utility functions to check state of UKM set-up.

ukm::SourceId DownloadUkmHelper::GetNewSourceId(GURL& url) {
  ukm::SourceId source_id = ukm_recorder_->GetNewSourceID();
  ukm_recorder_->UpdateSourceURL(source_id, url);
}

bool DownloadUkmHelper::CanLog(GURL& url) {
  return ukm_recorder_ && url.is_valid();
}
