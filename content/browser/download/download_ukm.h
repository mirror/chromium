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
  DownloadUkmHelper(ukm::UkmRecorder* ukm_recorder, const GURL& url);
  ~DownloadUkmHelper();

  void RecordDownloadStarted(int file_type, int num_streams, int component);

  void RecordDownloadInterrupted(int time_since_start, int reason);

  void RecordDownloadResumed(int time_since_start, int reason);

  void RecordDownloadEnded(int time_since_start,
                           int status,
                           int resulting_file_size,
                           int bytes_wasted,
                           int change_in_file_size);

 private:
  bool CanLog();
  void GetNewSourceId();

  ukm::UkmRecorder* ukm_recorder_;
  const GURL& url_;
  ukm::SourceId source_id_;
};

}  // namespace download

#endif  // CONTENT_BROWSER_DOWNLOAD_DOWNLOAD_UKM_H_
