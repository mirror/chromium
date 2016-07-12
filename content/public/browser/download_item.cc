// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/browser/download_item.h"

#include "base/guid.h"
#include "base/logging.h"
#include "base/strings/string_util.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/download_job.h"

namespace content {

const uint32_t DownloadItem::kInvalidId = 0;

DownloadItem::DownloadItem(const std::string& guid,
                           uint32_t download_id,
                           RequestInfo request,
                           DownloadItemDelegate* delegate)
    : guid_(guid),
      download_id_(download_id),
      delegate_(delegate),
      request_(request) {
  DCHECK(base::IsValidGUID(guid_));
  DCHECK(base::IsAsciiUpper(guid_));
}

DownloadItem::~DownloadItem() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  FOR_EACH_OBSERVER(Observer, observers_, OnDownloadDestroyed(this));
}

void DownloadItem::AddObserver(Observer* observer) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  observers_.AddObserver(observer);
}

void DownloadItem::RemoveObserver(Observer* observer) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  observers_.RemoveObserver(observer);
}

void DownloadItem::UpdateObservers() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  FOR_EACH_OBSERVER(Observer, observers_, OnDownloadUpdated(this));
}

const GURL& DownloadItem::GetURL() const {
  return request_.url_chain.empty() ? GURL::EmptyGURL()
                                    : request_.url_chain.back();
}

const std::vector<GURL>& DownloadItem::GetUrlChain() const {
  return request_.url_chain;
}

const GURL& DownloadItem::GetOriginalUrl() const {
  return request_.url_chain.empty() ? GURL::EmptyGURL()
                                    : request_.url_chain.front();
}

const GURL& DownloadItem::GetReferrerUrl() const {
  return request_.referrer_url;
}

const GURL& DownloadItem::GetSiteUrl() const {
  return request_.site_url;
}

const GURL& DownloadItem::GetTabUrl() const {
  return request_.tab_url;
}

const GURL& DownloadItem::GetTabReferrerUrl() const {
  return request_.tab_referrer_url;
}

std::string DownloadItem::GetSuggestedFilename() const {
  return request_.suggested_filename;
}

std::string DownloadItem::GetContentDisposition() const {
  return response_.content_disposition;
}

std::string DownloadItem::GetMimeType() const {
  return response_.mime_type;
}

std::string DownloadItem::GetOriginalMimeType() const {
  return response_.original_mime_type;
}

std::string DownloadItem::GetRemoteAddress() const {
  return request_.remote_address;
}

bool DownloadItem::HasUserGesture() const {
  return request_.has_user_gesture;
};

ui::PageTransition DownloadItem::GetTransitionType() const {
  return request_.transition_type;
};

const std::string& DownloadItem::GetLastModifiedTime() const {
  return response_.last_modified_time;
}

const std::string& DownloadItem::GetETag() const {
  return response_.etag;
}

bool DownloadItem::IsSavePackageDownload() const {
  return job_->IsSavePackageDownload();
}

const base::FilePath& DownloadItem::GetFullPath() const {
  return destination_.current_path;
}

const base::FilePath& DownloadItem::GetTargetFilePath() const {
  return destination_.target_path;
}

const base::FilePath& DownloadItem::GetForcedFilePath() const {
  // TODO(asanka): Get rid of GetForcedFilePath(). We should instead just
  // require that clients respect GetTargetFilePath() if it is already set.
  return request_.forced_file_path;
}

base::FilePath DownloadItem::GetFileNameToReportUser() const {
  if (!destination_.display_name.empty())
    return destination_.display_name;
  return destination_.target_path.BaseName();
}

DownloadItem::TargetDisposition DownloadItem::GetTargetDisposition() const {
  return destination_.target_disposition;
}

const std::string& DownloadItem::GetHash() const {
  return destination_.hash;
}

bool DownloadItem::GetFileExternallyRemoved() const {
  return file_externally_removed_;
}

bool DownloadItem::IsDangerous() const {
  return (danger_type_ == DOWNLOAD_DANGER_TYPE_DANGEROUS_FILE ||
          danger_type_ == DOWNLOAD_DANGER_TYPE_DANGEROUS_URL ||
          danger_type_ == DOWNLOAD_DANGER_TYPE_DANGEROUS_CONTENT ||
          danger_type_ == DOWNLOAD_DANGER_TYPE_UNCOMMON_CONTENT ||
          danger_type_ == DOWNLOAD_DANGER_TYPE_DANGEROUS_HOST ||
          danger_type_ == DOWNLOAD_DANGER_TYPE_POTENTIALLY_UNWANTED);
}

DownloadDangerType DownloadItem::GetDangerType() const {
  return danger_type_;
}

bool DownloadItem::AllDataSaved() const {
  return destination_.all_data_saved;
}

int64_t DownloadItem::GetTotalBytes() const {
  return response_.total_bytes;
}

int64_t DownloadItem::GetReceivedBytes() const {
  return destination_.received_bytes;
}

base::Time DownloadItem::GetStartTime() const {
  return request_.start_time;
}

base::Time DownloadItem::GetEndTime() const {
  return destination_.end_time;
}

bool DownloadItem::GetOpenWhenComplete() const {
  return open_when_complete_;
}

bool DownloadItem::GetAutoOpened() const {
  return auto_opened_;
}

bool DownloadItem::GetOpened() const {
  return opened_;
}

void DownloadItem::SetOpenWhenComplete(bool open) {
  open_when_complete_ = open;
}

void DownloadItem::SetDisplayName(const base::FilePath& name) {
  destination_.display_name = name;
}

}  // namespace content
