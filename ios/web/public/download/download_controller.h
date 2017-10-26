// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_PUBLIC_DOWNLOAD_DOWNLOAD_CONTROLLER_H_
#define IOS_WEB_PUBLIC_DOWNLOAD_DOWNLOAD_CONTROLLER_H_

#include <Foundation/Foundation.h>
#include <string>

class GURL;

namespace web {

class BrowserState;
class DownloadControllerObserver;
class WebState;

class DownloadController {
 public:
  static DownloadController* FromBrowserState(BrowserState* browser_state);

  virtual void CreateDownloadTask(const WebState* web_state,
                                  NSString* identifier,
                                  const GURL& original_url,
                                  const std::string& content_disposition,
                                  int64_t total_bytes,
                                  const std::string& mime_type) = 0;

  virtual void AddObserver(DownloadControllerObserver* observer) = 0;
  virtual void RemoveObserver(DownloadControllerObserver* observer) = 0;

  virtual ~DownloadController() = default;
};

}  // namespace web

#endif  // IOS_WEB_PUBLIC_DOWNLOAD_DOWNLOAD_CONTROLLER_H_
