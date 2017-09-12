// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_DOWNLOAD_DOWNLOAD_BROWSERTEST_H_
#define CHROME_BROWSER_DOWNLOAD_DOWNLOAD_BROWSERTEST_H_

#include <stddef.h>

#include "base/macros.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/test/download_test_observer.h"
#include "content/public/test/test_file_error_injector.h"

// The base class for Chrome download browser test.
class DownloadTestBase : public InProcessBrowserTest {
 public:
  // Choice of navigation or direct fetch.  Used by |DownloadFileCheckErrors()|.
  enum DownloadMethod { DOWNLOAD_NAVIGATE, DOWNLOAD_DIRECT };

  // Information passed in to |DownloadFileCheckErrors()|.
  struct DownloadInfo {
    const char* starting_url;           // URL for initiating the download.
    const char* expected_download_url;  // Expected value of DI::GetURL(). Can
                                        // be different if |starting_url|
                                        // initiates a download from another
                                        // URL.
    DownloadMethod download_method;     // Navigation or Direct.
    // Download interrupt reason (NONE is OK).
    content::DownloadInterruptReason reason;
    bool show_download_item;  // True if the download item appears on the shelf.
    bool should_redirect_to_documents;  // True if we save it in "My Documents".
  };

  struct FileErrorInjectInfo {
    DownloadInfo download_info;
    content::TestFileErrorInjector::FileErrorInfo error_info;
  };

  DownloadTestBase();

  // content::BrowserTestBase implementation.
  void SetUpOnMainThread() override;
  void TearDownOnMainThread() override;

 protected:
  // Must be called after browser creation.  Creates a temporary
  // directory for downloads that is auto-deleted on destruction.
  // Returning false indicates a failure of the function, and should be asserted
  // in the caller.
  bool CreateAndSetDownloadsDirectory(Browser* browser);

  base::FilePath GetDownloadsDirectory();

  // Location of the file source (the place from which it is downloaded).
  base::FilePath OriginFile(const base::FilePath& file);

 private:
  // Returning false indicates a failure of the setup, and should be asserted
  // in the caller.
  bool InitialSetup();

  // Location of the test data.
  base::FilePath test_dir_;

  // Location of the downloads directory for these tests
  base::ScopedTempDir downloads_directory_;

  DISALLOW_COPY_AND_ASSIGN(DownloadTestBase);
};

// DownloadTestObserver subclass that observes a download until it transitions
// from IN_PROGRESS to another state, but only after StartObserving() is called.
class DownloadTestObserverNotInProgress : public content::DownloadTestObserver {
 public:
  DownloadTestObserverNotInProgress(content::DownloadManager* download_manager,
                                    size_t count);
  ~DownloadTestObserverNotInProgress() override;

  void StartObserving();

 private:
  bool IsDownloadInFinalState(content::DownloadItem* download) override;

  bool started_observing_;

  DISALLOW_COPY_AND_ASSIGN(DownloadTestObserverNotInProgress);
};

#endif  // CHROME_BROWSER_DOWNLOAD_DOWNLOAD_BROWSERTEST_H_
