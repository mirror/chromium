// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_PUBLIC_DOWNLOAD_DOWNLOAD_CONTROLLER_OBSERVER_H_
#define IOS_WEB_PUBLIC_DOWNLOAD_DOWNLOAD_CONTROLLER_OBSERVER_H_

#include "base/macros.h"
#include "base/memory/ref_counted.h"

namespace web {

class DownloadController;
class DownloadTask;
class WebState;

// Allows observing of creation, update and destruction of DownloadTask
// objects, as well as DownloadController destruction. All methods are called on
// UI thread.
class DownloadControllerObserver {
 public:
  // Called when renderer-initiated download was created or when client is
  // resuming previously suspended download by calling
  // DownloadController::CreateDownloadTask().
  //
  // Renderer-initiated download or download created with
  // DownloadController::CreateDownloadTask() call does not start automatically.
  // If the client wants to start the download it should set responce writer
  // via DownloadTask::SetResponseWriter() and call DownloadTask::Start().
  // Clients may call DownloadTask::GetSuggestedFilename() to get the filename
  // for the download and DownloadTask::GetTotalBytes() to get the estimated
  // size.
  virtual void OnDownloadCreated(const WebState*, scoped_refptr<DownloadTask>) {
  }

  // Called when download task has started, downloaded a chunk of data or
  // the download has been completed. Clients may call DownloadTask::IsDone() to
  // check if the task has completed, call DownloadTask::GetErrorCode() to check
  // if the download has failed, DownloadTask::GetPercentComplete() to check
  // the download progress, and DownloadTask::GetResponseWriter() to obtain the
  // downloaded data.
  virtual void OnDownloadUpdated(const WebState*, DownloadTask*) {}

  // Called when downaload task is about to be destroyed. Cliends should remove
  // all references to the given DownloadTask and stop using the task.
  virtual void OnDownloadDestroyed(const WebState*, DownloadTask*) {}

  // Called when DownloadController is about to be destroyed. Cliends should
  // remove self as observers, stop using this DownloadController. All retained
  // references to DownloadTask objects are still alive, but the downloads will
  // be cancelled and DownloadControllerObserver callbacks will not be called
  // again.
  virtual void OnDownloadControllerDestroyed(DownloadController*) {}

  DownloadControllerObserver() = default;
  virtual ~DownloadControllerObserver() = default;

 private:
  DISALLOW_COPY_AND_ASSIGN(DownloadControllerObserver);
};

}  // namespace web

#endif  // IOS_WEB_PUBLIC_DOWNLOAD_DOWNLOAD_CONTROLLER_OBSERVER_H_
