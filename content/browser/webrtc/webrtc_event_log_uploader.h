// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_WEBRTC_WEBRTC_EVENT_LOG_UPLOADER_H_
#define CONTENT_BROWSER_WEBRTC_WEBRTC_EVENT_LOG_UPLOADER_H_

#include "base/files/file_path.h"

namespace content {

// A class implementing this interace can register for notification of an
// upload's eventual result (success/failure).
class WebRtcEventLogUploaderObserver {
 public:
  virtual ~WebRtcEventLogUploaderObserver() = default;
  virtual void OnWebRtcEventLogUploadSuccess() = 0;
  virtual void OnWebRtcEventLogUploadFailure() = 0;
};

class WebRtcEventLogUploader {
 public:
  WebRtcEventLogUploader(WebRtcEventLogUploaderObserver* observer,
                         base::FilePath path);
  virtual ~WebRtcEventLogUploader();

  // private:  // TODO: !!! Uncomment.
  WebRtcEventLogUploaderObserver* const observer_;
  const base::FilePath path_;
};

}  // namespace content

#endif  // CONTENT_BROWSER_WEBRTC_WEBRTC_EVENT_LOG_UPLOADER_H_
