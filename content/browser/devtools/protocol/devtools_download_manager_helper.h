// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_DEVTOOLS_PROTOCOL_DEVTOOLS_DOWNLOAD_MANAGER_HELPER_H_
#define CONTENT_BROWSER_DEVTOOLS_PROTOCOL_DEVTOOLS_DOWNLOAD_MANAGER_HELPER_H_

#include "content/public/browser/web_contents_user_data.h"

namespace content {

class WebContents;

namespace protocol {

// Per-WebContents class to handle DevTools download requests.
class DevToolsDownloadManagerHelper
    : public content::WebContentsUserData<DevToolsDownloadManagerHelper> {
 public:
  enum class DownloadBehavior {
    // All downloads are denied.
    DENY,

    // All downloads are accepted.
    ALLOW
  };

  ~DevToolsDownloadManagerHelper() override;

  DownloadBehavior GetDownloadBehavior();
  void SetDownloadBehavior(DownloadBehavior behavior);
  std::string GetDownloadPath();
  void SetDownloadPath(const std::string& path);

 private:
  explicit DevToolsDownloadManagerHelper(content::WebContents* web_contents);
  friend class content::WebContentsUserData<DevToolsDownloadManagerHelper>;

  DownloadBehavior download_behavior_;
  std::string download_path_;
};

}  // namespace protocol
}  // namespace content

#endif  // CONTENT_BROWSER_DEVTOOLS_PROTOCOL_DEVTOOLS_DOWNLOAD_MANAGER_HELPER_H_