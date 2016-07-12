// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_BROWSER_DOWNLOAD_JOB_H_
#define CONTENT_PUBLIC_BROWSER_DOWNLOAD_JOB_H_

#include <string>

#include "content/public/browser/download_item.h"

namespace content {

class BrowserContext;
class WebContents;

class DownloadJob {
 public:
  DownloadJob();
  virtual ~DownloadJob();

  // Only valid between OnAttached() and OnBeforeDetach(), inclusive.
  DownloadItem* download_item() { return download_item_; }

  // Called by DownloadItem: ---------------------------------------------------
  virtual void OnAttached() = 0;
  virtual void OnBeforeDetach() = 0;

  virtual void Cancel(bool user_cancel) = 0;
  virtual void Pause() = 0;
  virtual void Resume() = 0;

  virtual bool CanOpen() const = 0;
  virtual bool CanResume() const = 0;
  virtual bool CanShowInFolder() const = 0;
  virtual bool IsActive() const = 0;
  virtual bool IsPaused() const = 0;

  // Should return a number between 0 and 100 indicating the progress of the
  // download operation. Return -1 to indicate that the progress is
  // indeterminate. Only applicable if IsActive() is true.
  virtual int PercentComplete() const = 0;

  // Return an estimate of the current download speed in bytes per second.
  // Return -1 if the speed is unknown.
  virtual int64_t CurrentSpeed() const = 0;

  // Estimate of how long it will be before the download is considered complete.
  // If an estimate is available, |remaining| should be set to the estimated
  // time remaining and return true. Otherwise return false to indicate that an
  // estimate is not available.
  virtual bool TimeRemaining(base::TimeDelta* remaining) const = 0;

  // Return the WebContents associated with the download. Usually used to
  // associate a browser window for any UI that needs to be displayed to the
  // user. Return nullptr if the download is not associated with an active
  // WebContents.
  virtual WebContents* GetWebContents() const = 0;

  virtual std::string DebugString(bool verbose) const = 0;

  // To be deprecated.
  virtual bool IsSavePackageDownload() const = 0;

 protected:
  // RequestInfo can only be set via the DownloadItem constructor. It can't be
  // modified afterwards.
  const DownloadItem::RequestInfo& request_info() {
    return download_item_->request_;
  }

  // ResponseInfo can only modified via StartedSavingResponse().
  const DownloadItem::ResponseInfo& response_info() {
    return download_item_->response_;
  }

  // Mutable DestinationInfo accessor. Call download_item()->UpdateObservers()
  // after changing anything. Note that UpdateObservers() is invoked
  // synchronously and is generally expensive. If there are multiple changes
  // that should be considered atomic, then all such changes should be made
  // prior to calling UpdateObservers().
  DownloadItem::DestinationInfo& destination_info() {
    return download_item_->destination_;
  }

  // Mark the download as being in-progress. The returned |response| parameters
  // will be used as the current ResponseInfo for the download.
  void StartedSavingResponse(DownloadItem::ResponseInfo response);

  // Sets the download danger type and calls UpdateObservers().
  void SetDangerType(DownloadDangerType danger_type);

  // Sets the DownloadItem state to INTERRUPTED and sets |reason| as the last
  // interrupt reason. Automatically calls UpdateObservers().
  void Interrupt(DownloadInterruptReason reason);

  // Mark the download as being successfully completed. Automatically calls
  // UpdateObservers().
  void DownloadComplete();

  // Marks the file as being removed externally and calls UpdateObservers().
  void SetFileExternallyRemoved(bool externally_removed);

 private:
  friend class DownloadItem;

  DownloadItem* download_item_ = nullptr;
};

}  // namespace content

#endif  // CONTENT_PUBLIC_BROWSER_DOWNLOAD_JOB_H_
