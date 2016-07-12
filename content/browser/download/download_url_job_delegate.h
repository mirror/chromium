// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_DOWNLOAD_DOWNLOAD_URL_JOB_DELEGATE_H_
#define CONTENT_BROWSER_DOWNLOAD_DOWNLOAD_URL_JOB_DELEGATE_H_

#include <stdint.h>

#include "base/callback.h"
#include "base/files/file_path.h"
#include "base/macros.h"
#include "content/common/content_export.h"
#include "content/public/browser/download_danger_type.h"
#include "content/public/browser/download_item.h"
#include "content/public/browser/download_url_parameters.h"

namespace content {
class DownloadUrlJob;
class BrowserContext;

// Delegate for operations that a DownloadUrlJob can't do for itself.
// The base implementation of this class does nothing (returning false
// on predicates) so interfaces not of interest to a derived class may
// be left unimplemented.
class CONTENT_EXPORT DownloadUrlJobDelegate {
 public:
  typedef base::Callback<void(
      const base::FilePath&,            // Target path
      DownloadItem::TargetDisposition,  // overwrite/uniquify target
      DownloadDangerType,
      const base::FilePath&  // Intermediate file path
      )>
      DownloadTargetCallback;

  // The boolean argument indicates whether or not the download was
  // actually opened.
  typedef base::Callback<void(bool)> ShouldOpenDownloadCallback;

  DownloadUrlJobDelegate();
  virtual ~DownloadUrlJobDelegate();

  // Used for catching use-after-free errors.
  void Attach();
  void Detach();

  // Request determination of the download target from the delegate.
  virtual void DetermineDownloadTarget(DownloadUrlJob* download,
                                       const DownloadTargetCallback& callback);

  // Allows the delegate to delay completion of the download.  This function
  // will either return true (if the download may complete now) or will return
  // false and call the provided callback at some future point.  This function
  // may be called repeatedly.
  virtual bool ShouldCompleteDownload(DownloadUrlJob* download,
                                      const base::Closure& complete_callback);

  // Allows the delegate to override the opening of a download. If it returns
  // true then it's reponsible for opening the item.
  virtual bool ShouldOpenDownload(DownloadUrlJob* download,
                                  const ShouldOpenDownloadCallback& callback);

  // Tests if a file type should be opened automatically.
  virtual bool ShouldOpenFileBasedOnExtension(const base::FilePath& path);

  // Checks whether a downloaded file still exists and updates the
  // file's state if the file is already removed.
  // The check may or may not result in a later asynchronous call
  // to OnDownloadedFileRemoved().
  virtual void CheckForFileRemoval(DownloadItem* download_item);

  // Return a GUID string used for identifying the application to the system AV
  // function for scanning downloaded files. If no GUID is provided or if the
  // provided GUID is invalid, then the appropriate quarantining will be
  // performed manually without passing the download to the system AV function.
  //
  // This GUID is only used on Windows.
  virtual std::string GetApplicationClientIdForFileScanning() const;

  // Called when an interrupted download is resumed.
  virtual void ResumeInterruptedDownload(
      std::unique_ptr<content::DownloadUrlParameters> params,
      uint32_t id);

  // For contextual issues like language and prefs.
  virtual BrowserContext* GetBrowserContext() const;

  // Opens the file associated with this download.
  virtual void OpenDownload(DownloadItem* download);

  // Shows the download via the OS shell.
  virtual void ShowDownloadInShell(DownloadItem* download);

  // Handle any delegate portions of a state change operation on the
  // DownloadItem.
  virtual void DownloadRemoved(DownloadItem* download);

  // Assert consistent state for delgate object at various transitions.
  virtual void AssertStateConsistent(DownloadUrlJob* download) const;

 private:
  // For "Outlives attached DownloadUrlJob" invariant assertion.
  int count_;

  DISALLOW_COPY_AND_ASSIGN(DownloadUrlJobDelegate);
};

}  // namespace content

#endif  // CONTENT_BROWSER_DOWNLOAD_DOWNLOAD_URL_JOB_DELEGATE_H_
