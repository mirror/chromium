// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_DOWNLOAD_DOWNLOAD_CONTROLLER_IMPL_H_
#define IOS_WEB_DOWNLOAD_DOWNLOAD_CONTROLLER_IMPL_H_

#include <Foundation/Foundation.h>
#include <set>

#include "base/memory/ref_counted.h"
#include "base/observer_list.h"
#include "base/supports_user_data.h"
#import "ios/web/download/download_task_impl.h"
#import "ios/web/public/download/download_controller.h"

namespace web {

class WebState;

class DownloadControllerImpl : public DownloadController,
                               public base::SupportsUserData::Data,
                               public DownloadTaskImpl::Delegate {
 public:
  DownloadControllerImpl();
  ~DownloadControllerImpl() override;

  scoped_refptr<DownloadTask> CreateDownloadTask(
      const WebState* web_state,
      NSString* identifier,
      const GURL& original_url,
      const std::string& content_disposition,
      int64_t total_bytes,
      const std::string& mime_type) override;

  // DownloadController overrides:
  void AddObserver(DownloadControllerObserver* observer) override;
  void RemoveObserver(DownloadControllerObserver* observer) override;

  // DownloadTaskImpl::Delegate overrides:
  void OnTaskUpdated(DownloadTaskImpl* task) override;
  void OnTaskDestroyed(DownloadTaskImpl* task) override;
  NSURLSession* CreateSession(NSString* identifier,
                              id<NSURLSessionDataDelegate> delegate,
                              NSOperationQueue* delegate_queue) override;

 private:
  // A list of observers. Weak references.
  base::ObserverList<DownloadControllerObserver, true> observers_;
  // Set of tasks which are currently alive.
  std::set<DownloadTaskImpl*> alive_tasks_;
};

}  // namespace web

#endif  // IOS_WEB_DOWNLOAD_DOWNLOAD_CONTROLLER_IMPL_H_
