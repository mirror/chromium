// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/download/download_url_job_delegate.h"

#include "base/logging.h"
#include "content/browser/download/download_url_job.h"

namespace content {

// Infrastructure in DownloadUrlJobDelegate to assert invariant that
// delegate always outlives all attached DownloadItemImpls.
DownloadUrlJobDelegate::DownloadUrlJobDelegate() : count_(0) {}

DownloadUrlJobDelegate::~DownloadUrlJobDelegate() {
  DCHECK_EQ(0, count_);
}

void DownloadUrlJobDelegate::Attach() {
  ++count_;
}

void DownloadUrlJobDelegate::Detach() {
  DCHECK_LT(0, count_);
  --count_;
}

void DownloadUrlJobDelegate::DetermineDownloadTarget(
    DownloadUrlJob* download_job,
    const DownloadTargetCallback& callback) {
  // TODO(rdsmith/asanka): Do something useful if forced file path is null.
  base::FilePath target_path(
      download_job->download_item()->GetForcedFilePath());
  callback.Run(target_path, DownloadItem::TARGET_DISPOSITION_OVERWRITE,
               DOWNLOAD_DANGER_TYPE_NOT_DANGEROUS, target_path);
}

bool DownloadUrlJobDelegate::ShouldCompleteDownload(
    DownloadUrlJob* download,
    const base::Closure& complete_callback) {
  return true;
}

bool DownloadUrlJobDelegate::ShouldOpenDownload(
    DownloadUrlJob* download,
    const ShouldOpenDownloadCallback& callback) {
  return false;
}

bool DownloadUrlJobDelegate::ShouldOpenFileBasedOnExtension(
    const base::FilePath& path) {
  return false;
}

void DownloadUrlJobDelegate::CheckForFileRemoval(
    DownloadUrlJob* download_item) {}

std::string DownloadUrlJobDelegate::GetApplicationClientIdForFileScanning()
    const {
  return std::string();
}

void DownloadUrlJobDelegate::ResumeInterruptedDownload(
    std::unique_ptr<DownloadUrlParameters> params,
    uint32_t id) {}

BrowserContext* DownloadUrlJobDelegate::GetBrowserContext() const {
  return NULL;
}

void DownloadUrlJobDelegate::OpenDownload(DownloadUrlJob* download) {}

void DownloadUrlJobDelegate::ShowDownloadInShell(DownloadUrlJob* download) {}

void DownloadUrlJobDelegate::DownloadRemoved(DownloadUrlJob* download) {}

void DownloadUrlJobDelegate::AssertStateConsistent(
    DownloadUrlJob* download) const {}

}  // namespace content
