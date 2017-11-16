// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/download/download_ukm.h"

#include "services/metrics/public/cpp/ukm_builders.h"

namespace download {

namespace {
// Return increment between the log of the values that belong in each bucket.
double CalcBucketIncrement() {
  double percent_error = 10;
  return log10(1 + percent_error / 100);
}

// Returns the number of the bucket that this value belongs in.
int CalcExponentialBucket(int value) {
  return static_cast<int>(floor(log10(value + 1) / CalcBucketIncrement()));
}

}  // namespace

DownloadUkmHelper::DownloadUkmHelper(const int download_id,
                                     ukm::UkmRecorder* ukm_recorder,
                                     const GURL& url)
    : download_id_(download_id), ukm_recorder_(ukm_recorder), url_(url) {
  // TODO(jming): Do we need to keep same source id?
  // When do we get a new source id?
  GetNewSourceId();
}

DownloadUkmHelper::~DownloadUkmHelper() = default;

void DownloadUkmHelper::RecordDownloadStarted(int component,
                                              int file_type,
                                              int num_streams) {
  if (!CanLog())
    return;

  ukm::builders::Download_Started(source_id_)
      .SetComponent(component)
      .SetDownloadId(download_id_)
      .SetFileType(file_type)
      .SetNumStreams(num_streams)
      .Record(ukm_recorder_);
}

void DownloadUkmHelper::RecordDownloadInterrupted(int reason,
                                                  int time_since_start) {
  if (!CanLog())
    return;

  ukm::builders::Download_Interrupted(source_id_)
      .SetDownloadId(download_id_)
      .SetReason(reason)
      .SetTimeSinceStart(time_since_start)
      .Record(ukm_recorder_);
}

void DownloadUkmHelper::RecordDownloadResumed(int reason,
                                              int time_since_start) {
  if (!CanLog())
    return;

  ukm::builders::Download_Resumed(source_id_)
      .SetDownloadId(download_id_)
      .SetReason(reason)
      .SetTimeSinceStart(time_since_start)
      .Record(ukm_recorder_);
}

void DownloadUkmHelper::RecordDownloadEnded(int bytes_wasted,
                                            int change_in_file_size,
                                            int status,
                                            int resulting_file_size,
                                            int time_since_start) {
  if (!CanLog())
    return;

  ukm::builders::Download_Ended(source_id_)
      .SetBytesWasted(bytes_wasted)
      .SetChangeInFileSize(CalcExponentialBucket(change_in_file_size))
      .SetDownloadId(download_id_)
      .SetResultingFileSize(CalcExponentialBucket(resulting_file_size))
      .SetStatus(status)
      .SetTimeSinceStart(time_since_start)
      .Record(ukm_recorder_);
}

void DownloadUkmHelper::GetNewSourceId() {
  if (!url_.is_valid()) {
    LOG(ERROR) << "Error getting source ID because of invalid URL.";
    return;
  }

  source_id_ = ukm_recorder_->GetNewSourceID();
  ukm_recorder_->UpdateSourceURL(source_id_, url_);
}

bool DownloadUkmHelper::CanLog() const {
  return ukm_recorder_ && url_.is_valid() && source_id_;
}

}  // namespace download
