// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_PUBLIC_DOWNLOAD_DOWNLOAD_CONTROLLER_H_
#define IOS_WEB_PUBLIC_DOWNLOAD_DOWNLOAD_CONTROLLER_H_

#include <Foundation/Foundation.h>
#include <string>

#include "base/macros.h"
#include "base/memory/scoped_refptr.h"

class GURL;

namespace web {

class BrowserState;
class DownloadControllerObserver;
class DownloadTask;
class WebState;

// Provides API for browser downloads. Must be used on the UI thread. Allows
// handling renderer-initiated download taks and resuming unfinished downloads
// after app relaunch.
//
// Handling renderer-initiated downloads example:
// class MyDownloadManager : public DownloadControllerObserver {
//  public:
//   MyDownloadManager(BrowserState* state) : _state(state) {
//     DownloadController::FromBrowserState(state)->AddObserver(this);
//   }
//   void OnDownloadCreated(
//       const WebState*, scoped_refptr<DownloadTask> task) override {
//     task->SetResponseWriter(GetURLFetcherFileWriter());
//     task->Start();
//     .. retain |task| until download is complete...
//   }
//   void OnDownloadUpdated(const WebState*, DownloadTask* task) override {
//     if (task->IsDone()) {
//       .. release |task| as download is complete..
//     } else {
//       ShowProgress(task->GetPercentComplete());
//     }
//   }
//   void OnDownloadControllerDestroyed(DownloadController*) override {
//     DownloadController::FromBrowserState(_state)->RemoveObserver(this);
//   }
// };
//
// Resuming unfinished downloads after app relaunch example:
// @interface MyAppDelegate : UIResponder <UIApplicationDelegate>
// @end
// @implementation AppDelegate
// - (void)application:(UIApplication *)application
//     handleEventsForBackgroundURLSession:(NSString *)identifier
//                       completionHandler:(void (^)())completionHandler {
//   MyDownloadInfo* info = [self storedDownloadInfoForIdentifier:identifier];
//   // Retrieve the information about download task and create the download.
//   auto* controller = DownloadController::FromBrowserState(self.state);
//   scoped_refptr<DownloadTask> task = controller->CreateDownloadTask(
//       [self webStateAtIndex:info.webStateIndex],
//       identifier,
//       info.originalURL,
//       info.contentDisposition,
//       info.totalBytes,
//       info.MIMEType);
//   );
//   task->SetResponseWriter(GetURLFetcherFileWriter(info.fileName));
//   task->Start();
// }
// - (void)applicationWillTerminate:(UIApplication *)application {
//   for (DownloadTask* task : self.downloadTasks) {
//     [MyDownloadInfo storeDownloadInfoForTask:task];
//   }
// }
// @end
//
class DownloadController {
 public:
  // Returns DownloadController for the given |browser_state|. |browser_state|
  // must not be null.
  static DownloadController* FromBrowserState(BrowserState* browser_state);

  // Creates a new download task. Clients may call this method to resume the
  // download after the app cold start. Clients must not to call this method to
  // initiate a renderer-initiated download (those downloads are created
  // automatically).
  // In order to resume the download after the app cold start clients have to
  // pass |identifier| obtained from
  // application:handleEventsForBackgroundURLSession:completionHandler:
  // UIApplicationDelegate callback. The rest of arguments should be taken
  // from DownloadTask, which was suspended when the app has been terminated.
  // In order to resume the download, clients must persist all DownloadTask data
  // for each unfinished download on disk.
  virtual scoped_refptr<DownloadTask> CreateDownloadTask(
      const WebState* web_state,
      NSString* identifier,
      const GURL& original_url,
      const std::string& content_disposition,
      int64_t total_bytes,
      const std::string& mime_type) = 0;

  // Adds and Removes DownloadControllerObserver. Clients must remove self from
  // observers in DownloadControllerObserver::OnDownloadControllerDestroyed().
  virtual void AddObserver(DownloadControllerObserver* observer) = 0;
  virtual void RemoveObserver(DownloadControllerObserver* observer) = 0;

  DownloadController() = default;
  virtual ~DownloadController() = default;

 private:
  DISALLOW_COPY_AND_ASSIGN(DownloadController);
};

}  // namespace web

#endif  // IOS_WEB_PUBLIC_DOWNLOAD_DOWNLOAD_CONTROLLER_H_
