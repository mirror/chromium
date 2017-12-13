// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Holds helpers for gathering UMA stats about downloads.

#ifndef CONTENT_BROWSER_DOWNLOAD_DOWNLOAD_UKM_H_
#define CONTENT_BROWSER_DOWNLOAD_DOWNLOAD_UKM_H_

#include <stddef.h>
#include <stdint.h>
#include <string>

#include "base/optional.h"
#include "content/common/content_export.h"
#include "services/metrics/public/cpp/ukm_recorder.h"

namespace content {

class CONTENT_EXPORT DownloadUkmHelper {
 public:
  DownloadUkmHelper(int download_id, const GURL url);
  ~DownloadUkmHelper();

  // Calculate which exponential bucket the value falls in. This is used to mask
  // the actual value of the metric due to privacy concerns for certain metrics
  // that could trace back the user's exact actions.
  int CalcExponentialBucket(int value);

  // Record when the download has started.
  void RecordDownloadStarted(int file_type);

  // Record when the download is interrupted.
  void RecordDownloadInterrupted(base::Optional<int> change_in_file_size,
                                 int reason,
                                 int resulting_file_size,
                                 const base::TimeDelta& time_since_start);

  // Record when the download is resumed.
  void RecordDownloadResumed(int mode, const base::TimeDelta& time_since_start);

  // Record when the download is completed.
  void RecordDownloadCompleted(int resulting_file_size,
                               const base::TimeDelta& time_since_start);

 private:
  bool CanLog() const;
  void GetNewSourceId();

  const int download_id_;
  ukm::SourceId source_id_;
  const GURL url_;

  DISALLOW_COPY_AND_ASSIGN(DownloadUkmHelper);
};

}  // namespace content

#endif  // CONTENT_BROWSER_DOWNLOAD_DOWNLOAD_UKM_H_
