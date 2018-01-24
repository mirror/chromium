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

// A sublcass of this interface would take ownership of a file, and either
// upload it to a remote server (actual implementation), or pretend to do
// so (in unit tests). It will typically take on an observer of type
// WebRtcEventLogUploaderObserver, and inform it of the success or failure
// of the upload.
class WebRtcEventLogUploader {
 public:
  virtual ~WebRtcEventLogUploader() = default;
};

class WebRtcEventLogUploaderImpl : public WebRtcEventLogUploader {
 public:
  WebRtcEventLogUploaderImpl(WebRtcEventLogUploaderObserver* observer,
                             base::FilePath path);
  ~WebRtcEventLogUploaderImpl() override;

 private:
  // TODO(eladalon): Provide an actual implementation. https://crbug.com/775415
};

}  // namespace content

#endif  // CONTENT_BROWSER_WEBRTC_WEBRTC_EVENT_LOG_UPLOADER_H_
