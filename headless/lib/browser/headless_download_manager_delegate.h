// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef HEADLESS_LIB_BROWSER_HEADLESS_DOWNLOAD_MANAGER_DELEGATE_H_
#define HEADLESS_LIB_BROWSER_HEADLESS_DOWNLOAD_MANAGER_DELEGATE_H_

#include <stdint.h>

#include "base/callback_forward.h"
#include "base/compiler_specific.h"
#include "base/files/file_util.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "content/public/browser/download_manager_delegate.h"
#include "headless/public/headless_export.h"

namespace content {
class DownloadManager;
}

namespace headless {

class HEADLESS_EXPORT HeadlessDownloadManagerDelegate
    : public content::DownloadManagerDelegate {
 public:
  enum class DownloadBehavior {
    // All downloads are denied.
    DENY,

    // All downloads are accepted.
    ALLOW
  };

  HeadlessDownloadManagerDelegate();
  ~HeadlessDownloadManagerDelegate() override;

  void SetDownloadManager(content::DownloadManager* manager);

  void Shutdown() override;

  bool DetermineDownloadTarget(
      content::DownloadItem* download,
      const content::DownloadTargetCallback& callback) override;

  bool ShouldOpenDownload(
      content::DownloadItem* item,
      const content::DownloadOpenDelayedCallback& callback) override;

  void GetNextId(const content::DownloadIdCallback& callback) override;

  void SetDownloadBehavior(DownloadBehavior behavior);
  void SetDownloadDirectory(const base::FilePath& path);

 private:
  friend class base::RefCountedThreadSafe<HeadlessDownloadManagerDelegate>;

  typedef base::Callback<void(const base::FilePath&)>
      FilenameDeterminedCallback;

  static void GenerateFilename(const GURL& url,
                               const std::string& content_disposition,
                               const std::string& suggested_filename,
                               const std::string& mime_type,
                               const base::FilePath& suggested_directory,
                               const FilenameDeterminedCallback& callback);

  void OnDownloadPathGenerated(uint32_t download_id,
                               const content::DownloadTargetCallback& callback,
                               const base::FilePath& suggested_path);

  content::DownloadManager* download_manager_;
  base::FilePath default_download_path_;
  DownloadBehavior download_behavior_;
  base::WeakPtrFactory<HeadlessDownloadManagerDelegate> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(HeadlessDownloadManagerDelegate);
};

}  // namespace headless

#endif  // HEADLESS_LIB_BROWSER_HEADLESS_DOWNLOAD_MANAGER_DELEGATE_H_
