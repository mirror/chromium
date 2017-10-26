// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_PUBLIC_DOWNLOAD_DOWNLOAD_TASK_H_
#define IOS_WEB_PUBLIC_DOWNLOAD_DOWNLOAD_TASK_H_

#include <Foundation/Foundation.h>
#include <stdint.h>
#include <string>

#include "base/memory/ref_counted.h"
#include "base/strings/string16.h"

namespace base {
class Time;
}  // namespace base
class GURL;

namespace net {
class URLFetcherResponseWriter;
}  // namespace net

namespace web {

class DownloadTask : public base::RefCounted<DownloadTask> {
 public:
  virtual void Start() = 0;
  virtual net::URLFetcherResponseWriter* GetResponseWriter() const = 0;
  virtual void SetResponseWriter(
      std::unique_ptr<net::URLFetcherResponseWriter> writer) = 0;
  virtual NSString* GetIndentifier() const = 0;
  virtual const GURL& GetOriginalUrl() const = 0;
  virtual bool IsDone() const = 0;
  virtual int GetErrorCode() const = 0;
  // Total number of expected bytes. Returns -1 if the total size is unknown.
  virtual int64_t GetTotalBytes() const = 0;
  virtual int GetPercentComplete() const = 0;
  // Content-Disposition header value from HTTP response.
  virtual std::string GetContentDisposition() const = 0;
  // Effective MIME type of downloaded content.
  virtual std::string GetMimeType() const = 0;
  // For downloads initiated via <a download>, this is the suggested download
  // filename from the download attribute.
  virtual base::string16 GetSuggestedFilename() const = 0;

 protected:
  friend class RefCounted<DownloadTask>;
  virtual ~DownloadTask() = default;
};

}  // namespace web

#endif  // IOS_WEB_PUBLIC_DOWNLOAD_DOWNLOAD_TASK_H_
