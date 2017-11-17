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

DownloadUkmHelper::DownloadUkmHelper(const int download_id, const GURL& url)
    : download_id_(download_id), url_(url) {
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

  ukm::UkmRecorder* ukm_recorder = ukm::UkmRecorder::Get();
  ukm::builders::Download_Started(source_id_)
      .SetComponent(component)
      .SetDownloadId(download_id_)
      .SetFileType(file_type)
      .SetNumStreams(num_streams)
      .Record(ukm_recorder);
}

void DownloadUkmHelper::RecordDownloadInterrupted(
    int reason,
    base::TimeDelta time_since_start) {
  if (!CanLog())
    return;

  ukm::UkmRecorder* ukm_recorder = ukm::UkmRecorder::Get();
  ukm::builders::Download_Interrupted(source_id_)
      .SetDownloadId(download_id_)
      .SetReason(reason)
      .SetTimeSinceStart(time_since_start.InMilliseconds())
      .Record(ukm_recorder);
}

void DownloadUkmHelper::RecordDownloadResumed(
    int reason,
    base::TimeDelta time_since_start) {
  if (!CanLog())
    return;

  ukm::UkmRecorder* ukm_recorder = ukm::UkmRecorder::Get();
  ukm::builders::Download_Resumed(source_id_)
      .SetDownloadId(download_id_)
      .SetReason(reason)
      .SetTimeSinceStart(time_since_start.InMilliseconds())
      .Record(ukm_recorder);
}

void DownloadUkmHelper::RecordDownloadEnded(int bytes_wasted,
                                            int change_in_file_size,
                                            int resulting_file_size,
                                            int status,
                                            base::TimeDelta time_since_start) {
  if (!CanLog())
    return;

  ukm::UkmRecorder* ukm_recorder = ukm::UkmRecorder::Get();
  ukm::builders::Download_Ended(source_id_)
      .SetBytesWasted(bytes_wasted)
      .SetChangeInFileSize(CalcExponentialBucket(change_in_file_size))
      .SetDownloadId(download_id_)
      .SetResultingFileSize(CalcExponentialBucket(resulting_file_size))
      .SetStatus(status)
      .SetTimeSinceStart(time_since_start.InMilliseconds())
      .Record(ukm_recorder);
}

void DownloadUkmHelper::GetNewSourceId() {
  if (!url_.is_valid()) {
    LOG(ERROR) << "Error getting source ID because of invalid URL.";
    return;
  }

  ukm::UkmRecorder* ukm_recorder = ukm::UkmRecorder::Get();
  source_id_ = ukm_recorder->GetNewSourceID();
  ukm_recorder->UpdateSourceURL(source_id_, url_);
}

bool DownloadUkmHelper::CanLog() const {
  return url_.is_valid() && source_id_;
}

}  // namespace download
