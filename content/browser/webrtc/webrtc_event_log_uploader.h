// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_WEBRTC_WEBRTC_EVENT_LOG_UPLOADER_H_
#define CONTENT_BROWSER_WEBRTC_WEBRTC_EVENT_LOG_UPLOADER_H_

#include <memory>

#include "base/files/file_path.h"

namespace content {

// A class implementing this interace can register for notification of an
// upload's eventual result (success/failure).
class WebRtcEventLogUploaderObserver {
 public:
  virtual ~WebRtcEventLogUploaderObserver() = default;
  virtual void OnWebRtcEventLogUploadComplete(bool upload_successful) = 0;
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

// Since we'll need more than one instance of the abstract
// WebRtcEventLogUploader, we'll need an abstract factory for it.
class WebRtcEventLogUploaderFactory {
 public:
  virtual ~WebRtcEventLogUploaderFactory() = default;

  virtual std::unique_ptr<WebRtcEventLogUploader> Create(
      const base::FilePath& log_file) const = 0;
};

class WebRtcEventLogUploaderImpl : public WebRtcEventLogUploader {
 public:
  WebRtcEventLogUploaderImpl(WebRtcEventLogUploaderObserver* observer,
                             const base::FilePath& path);
  ~WebRtcEventLogUploaderImpl() override;
};

class WebRtcEventLogUploaderImplFactory : public WebRtcEventLogUploaderFactory {
 public:
  explicit WebRtcEventLogUploaderImplFactory(
      WebRtcEventLogUploaderObserver* observer);
  ~WebRtcEventLogUploaderImplFactory() override = default;

  std::unique_ptr<WebRtcEventLogUploader> Create(
      const base::FilePath& log_file) const override;

 private:
  WebRtcEventLogUploaderObserver* const observer_;
};

}  // namespace content

#endif  // CONTENT_BROWSER_WEBRTC_WEBRTC_EVENT_LOG_UPLOADER_H_
