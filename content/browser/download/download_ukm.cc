// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/download/download_ukm.h"

#include "services/metrics/public/cpp/ukm_builders.h"

namespace download {

DownloadUkmHelper::DownloadUkmHelper(ukm::UkmRecorder* ukm_recorder,
                                     const GURL& url)
    : ukm_recorder_(ukm_recorder), url_(url) {
  GetNewSourceId();
}

DownloadUkmHelper::~DownloadUkmHelper() = default;

void DownloadUkmHelper::RecordDownloadStarted(int file_type,
                                              int num_streams,
                                              int component) {
  if (!CanLog())
    return;

  // TODO(jming): Figure out how to get entry id as parent entry id.
  ukm::builders::Download_Started(source_id_)
      .SetFileType(file_type)
      .SetNumStreams(num_streams)
      .SetComponent(component)
      .Record(ukm_recorder_);
}

void DownloadUkmHelper::RecordDownloadInterrupted(int time_since_start,
                                                  int reason) {
  if (!CanLog())
    return;

  // TODO(jming): Figure out how to pass in parent entry id.
  ukm::builders::Download_Interrupted(source_id_)
      .SetTimeSinceStart(file_type)
      .SetReason(reason)
      .Record(ukm_recorder_);
}

void DownloadUkmHelper::RecordDownloadResumed(int time_since_start,
                                              int reason) {
  if (!CanLog())
    return;

  // TODO(jming): Figure out how to pass in parent entry id.
  ukm::builders::Download_Resumed(source_id_)
      .SetTimeSinceStart(file_type)
      .SetReason(reason)
      .Record(ukm_recorder_);
}

void DownloadUkmHelper::RecordDownloadEnded(int time_since_start,
                                            int status,
                                            int resulting_file_size,
                                            int bytes_wasted,
                                            int change_in_file_size) {
  if (!CanLog())
    return;

  // TODO(jming): Figure out how to pass in parent entry id.
  ukm::builders::Download_Ended(source_id_)
      .SetTimeSinceStart(time_since_start)
      .SetStatus(status)
      .SetResultingFileSize(resulting_file_size)
      .SetBytesWasted(bytes_wasted)
      .SetChangeInFileSize(change_in_file_size)
      .Record(ukm_recorder_);
}

void DownloadUkmHelper::GetNewSourceId() {
  ukm::SourceId source_id_ = ukm_recorder_->GetNewSourceID();
  ukm_recorder_->UpdateSourceURL(source_id_, url_);
}

bool DownloadUkmHelper::CanLog() {
  return ukm_recorder_ && url_.is_valid() && source_id_;
}

}  // namespace download
