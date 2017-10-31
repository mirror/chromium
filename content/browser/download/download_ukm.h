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

namespace download_ukm {

class DownloadUkmHelper {
 public:
  DownloadUkmHelper(ukm::UkmRecorder* ukm_recorder, const GURL& url);
  ~DownloadUkmHelper();

  void RecordDownloadStarted(int file_type,
                             int num_streams,
                             int file_size,
                             int component);

  void RecordDownloadStartedFileType(int file_type);

 private:
  bool CanLog();
  void GetNewSourceId();

  ukm::UkmRecorder* ukm_recorder_;
  const GURL& url_;
  ukm::SourceId source_id_;
};

}  // namespace download_ukm

#endif  // CONTENT_BROWSER_DOWNLOAD_DOWNLOAD_UKM_H_
