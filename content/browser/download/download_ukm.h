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

namespace content {

class DownloadUkmHelper {
 public:
  explicit DownloadUkmHelper(ukm::UkmRecorder* ukm_recorder);

  ukm::SourceId getNewSourceId();

  void RecordDownloadStarted(ukm::SourceId source_id,
                             int file_type,
                             int num_streams,
                             int file_size,
                             int component);

  void RecordDownloadStartedFileType(ukm::SourceId source_id, int file_type);

 private:
  bool CanLog(GURL& url);

  ukm::UkmRecorder* ukm_recorder_;
  ukm::SourceId source_id_ = -1;
  GURL url_;
};

}  // namespace content

#endif  // CONTENT_BROWSER_DOWNLOAD_DOWNLOAD_UKM_H_
