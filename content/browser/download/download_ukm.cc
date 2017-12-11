// Copyright 2017 The Chromium Authors. All rights reserved.
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

DownloadUkmHelper::DownloadUkmHelper(int download_id, const GURL& url)
    : download_id_(download_id), url_(url) {
  // TODO(jming): Do we need to keep same source id?
  // When do we get a new source id?
  GetNewSourceId();
}

DownloadUkmHelper::~DownloadUkmHelper() = default;

void DownloadUkmHelper::RecordDownloadStarted(int file_type) {
  if (!CanLog())
    return;

  ukm::UkmRecorder* ukm_recorder = ukm::UkmRecorder::Get();
  ukm::builders::Download_Started(source_id_)
      .SetDownloadId(download_id_)
      .SetFileType(file_type)
      .Record(ukm_recorder);
}

void DownloadUkmHelper::RecordDownloadInterrupted(
    base::Optional<int> change_in_file_size,
    int reason,
    int resulting_file_size,
    const base::TimeDelta& time_since_start) {
  if (!CanLog())
    return;

  ukm::UkmRecorder* ukm_recorder = ukm::UkmRecorder::Get();
  if (change_in_file_size.has_value()) {
    ukm::builders::Download_Interrupted(source_id_)
        .SetChangeInFileSize(CalcExponentialBucket(change_in_file_size.value()))
        .SetDownloadId(download_id_)
        .SetReason(reason)
        .SetResultingFileSize(CalcExponentialBucket(resulting_file_size))
        .SetTimeSinceStart(time_since_start.InMilliseconds())
        .Record(ukm_recorder);
  } else {
    ukm::builders::Download_Interrupted(source_id_)
        .SetDownloadId(download_id_)
        .SetReason(reason)
        .SetResultingFileSize(CalcExponentialBucket(resulting_file_size))
        .SetTimeSinceStart(time_since_start.InMilliseconds())
        .Record(ukm_recorder);
  }
}

void DownloadUkmHelper::RecordDownloadResumed(
    int mode,
    const base::TimeDelta& time_since_start) {
  if (!CanLog())
    return;

  ukm::UkmRecorder* ukm_recorder = ukm::UkmRecorder::Get();
  ukm::builders::Download_Resumed(source_id_)
      .SetDownloadId(download_id_)
      .SetMode(mode)
      .SetTimeSinceStart(time_since_start.InMilliseconds())
      .Record(ukm_recorder);
}

void DownloadUkmHelper::RecordDownloadCompleted(
    int resulting_file_size,
    const base::TimeDelta& time_since_start) {
  if (!CanLog())
    return;

  ukm::UkmRecorder* ukm_recorder = ukm::UkmRecorder::Get();
  ukm::builders::Download_Completed(source_id_)
      .SetDownloadId(download_id_)
      .SetResultingFileSize(CalcExponentialBucket(resulting_file_size))
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
