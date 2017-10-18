// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_PUBLIC_DOWNLOAD_DOWNLOAD_CONTROLLER_OBSERVER_H_
#define IOS_WEB_PUBLIC_DOWNLOAD_DOWNLOAD_CONTROLLER_OBSERVER_H_

#include "base/memory/ref_counted.h"

namespace web {

class DownloadTask;
class WebState;

class DownloadControllerObserver {
 public:
  virtual void OnDownloadCreated(const WebState*, scoped_refptr<DownloadTask>) {
  }
  virtual void OnDownloadUpdated(const WebState*, DownloadTask*) {}
  virtual void OnDownloadDestroyed(const WebState*, DownloadTask*) {}

  virtual ~DownloadControllerObserver() = default;
};

}  // namespace web

#endif  // IOS_WEB_PUBLIC_DOWNLOAD_DOWNLOAD_CONTROLLER_OBSERVER_H_
