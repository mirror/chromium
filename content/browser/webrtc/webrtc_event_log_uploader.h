// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_WEBRTC_WEBRTC_EVENT_LOG_UPLOADER_H_
#define CONTENT_BROWSER_WEBRTC_WEBRTC_EVENT_LOG_UPLOADER_H_

#include "base/files/file_path.h"

namespace content {

class WebRtcEventLogUploader {
 public:
  explicit WebRtcEventLogUploader(base::FilePath path);
  virtual ~WebRtcEventLogUploader();

 private:
  base::FilePath path_;
};

}  // namespace content

#endif  // CONTENT_BROWSER_WEBRTC_WEBRTC_EVENT_LOG_UPLOADER_H_
