// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Holds helpers for gathering UMA stats about downloads.

#ifndef CONTENT_BROWSER_DOWNLOAD_DOWNLOAD_UKM_H_
#define CONTENT_BROWSER_DOWNLOAD_DOWNLOAD_UKM_H_

#include <stddef.h>
#include <stdint.h>
#include <string>

#include "services/metrics/public/cpp/ukm_recorder.h"

namespace download {

class DownloadUkmHelper {
 public:
  DownloadUkmHelper(const int download_id,
                    ukm::UkmRecorder* ukm_recorder,
                    const GURL& url);
  ~DownloadUkmHelper();

  void RecordDownloadStarted(int component, int file_type, int num_streams);

  void RecordDownloadInterrupted(int reason, int time_since_start);

  void RecordDownloadResumed(int reason, int time_since_start);

  void RecordDownloadEnded(int bytes_wasted,
                           int change_in_file_size,
                           int status,
                           int resulting_file_size,
                           int time_since_start);

 private:
  bool CanLog() const;
  void GetNewSourceId();

  const int download_id_;
  ukm::SourceId source_id_;
  ukm::UkmRecorder* ukm_recorder_;
  const GURL& url_;
};

}  // namespace download

#endif  // CONTENT_BROWSER_DOWNLOAD_DOWNLOAD_UKM_H_
