// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_DOWNLOAD_WEB_DOWNLOAD_SERVICE_H_
#define IOS_CHROME_BROWSER_DOWNLOAD_WEB_DOWNLOAD_SERVICE_H_

#include <memory>

#include "base/macros.h"
#include "components/keyed_service/core/keyed_service.h"
#import "ios/web/public/download/download_controller_delegate.h"

namespace web {
class BrowserState;
class DownloadController;
class DownloadTask;
class WebState;
}  // namespace web

class BrowserDownloadService : public KeyedService,
                               web::DownloadControllerDelegate {
 public:
  explicit BrowserDownloadService(web::BrowserState* browser_state);
  ~BrowserDownloadService() override;

 private:
  // web::DownloadControllerDelegate overrides:
  void OnDownloadCreated(web::DownloadController*,
                         web::WebState*,
                         std::unique_ptr<web::DownloadTask>) override;

  void OnDownloadControllerDestroyed(web::DownloadController*) override;

  DISALLOW_COPY_AND_ASSIGN(BrowserDownloadService);
};

#endif  // IOS_CHROME_BROWSER_DOWNLOAD_WEB_DOWNLOAD_SERVICE_H_
