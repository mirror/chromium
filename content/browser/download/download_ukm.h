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
#include "services/metrics/public/cpp/ukm_recorder.h"

namespace download {

class DownloadUkmHelper {
 public:
  DownloadUkmHelper(int download_id, const GURL& url);
  ~DownloadUkmHelper();

  void RecordDownloadStarted(int file_type);

  void RecordDownloadInterrupted(base::Optional<int> change_in_file_size,
                                 int reason,
                                 int resulting_file_size,
                                 const base::TimeDelta& time_since_start);

  void RecordDownloadResumed(int mode, const base::TimeDelta& time_since_start);

  void RecordDownloadCompleted(int resulting_file_size,
                               const base::TimeDelta& time_since_start);

 private:
  bool CanLog() const;
  void GetNewSourceId();

  const int download_id_;
  ukm::SourceId source_id_;
  const GURL& url_;
};

}  // namespace download

#endif  // CONTENT_BROWSER_DOWNLOAD_DOWNLOAD_UKM_H_
