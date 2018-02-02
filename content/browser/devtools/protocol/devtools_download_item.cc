// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/devtools/protocol/devtools_download_item.h"

#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/files/file_util.h"
#include "base/format_macros.h"
#include "base/guid.h"
#include "base/logging.h"
#include "base/metrics/histogram_macros.h"
#include "base/stl_util.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task_runner_util.h"
#include "content/browser/download/download_file.h"
#include "content/browser/download/download_item_impl_delegate.h"
#include "content/browser/download/download_job_factory.h"
#include "content/browser/download/download_job_impl.h"
#include "content/browser/download/download_request_handle.h"
#include "content/browser/download/download_source_stream.h"
#include "content/browser/download/download_stats.h"
#include "content/browser/download/download_task_runner.h"
#include "content/browser/download/parallel_download_utils.h"
#include "content/browser/renderer_host/render_view_host_impl.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/browser/download_danger_type.h"
#include "content/public/browser/download_interrupt_reasons.h"
#include "content/public/browser/download_url_parameters.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/common/content_features.h"
#include "content/public/common/referrer.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_status_code.h"
#include "net/log/net_log.h"
#include "net/log/net_log_event_type.h"
#include "net/log/net_log_parameters_callback.h"
#include "net/log/net_log_source.h"
#include "net/traffic_annotation/network_traffic_annotation.h"

namespace content {
namespace protocol {
// Constructing for a regular download:
DevToolsDownloadItem::DevToolsDownloadItem(
    const content::DownloadCreateInfo& info)
    : info_(info.url_chain,
            info.referrer_url,
            info.site_url,
            info.tab_url,
            info.tab_referrer_url,
            base::UTF16ToUTF8(info.save_info->suggested_name),
            info.save_info->file_path,
            info.transition_type ? info.transition_type.value()
                                 : ui::PAGE_TRANSITION_LINK,
            info.has_user_gesture,
            info.remote_address,
            info.start_time),
      guid_(info.guid.empty() ? base::GenerateGUID() : info.guid),
      response_headers_(info.response_headers),
      content_disposition_(info.content_disposition),
      mime_type_(info.mime_type),
      original_mime_type_(info.original_mime_type),
      total_bytes_(info.total_bytes),
      last_modified_time_(info.last_modified),
      etag_(info.etag),
      state_(IN_PROGRESS),
      weak_ptr_factory_(this) {
  DETACH_FROM_SEQUENCE(sequence_checker_);
}

DevToolsDownloadItem::~DevToolsDownloadItem() {}

void DevToolsDownloadItem::Initialize(
    std::unique_ptr<DownloadManager::InputStream> stream,
    std::unique_ptr<DownloadRequestHandleInterface> req_handle) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (state_ != IN_PROGRESS)
    return;

  request_handle_ = std::move(req_handle);
  reading_ = false;
  stream_ =
      base::MakeUnique<content::DownloadSourceStream>(0, 0, std::move(stream));
  stream_->Initialize();
  stream_->RegisterDataReadyCallback(base::BindRepeating(
      &DevToolsDownloadItem::DataAvailable, weak_ptr_factory_.GetWeakPtr()));
}

void DevToolsDownloadItem::DataAvailable(MojoResult result) {
  if (reading_)
    ReadInternal();
}

void DevToolsDownloadItem::Read(scoped_refptr<net::IOBuffer> data,
                                size_t size,
                                ReadCompleteCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (state_ != IN_PROGRESS || !stream_ || reading_)
    return;

  reading_ = true;
  current_buffer_read_ = data;
  current_buffer_size_ = size;
  current_bytes_remaining_ = size;
  current_offset_ = 0;

  callback_ = std::move(callback);
  ReadInternal();
}

void DevToolsDownloadItem::ReadInternal() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  content::DownloadSourceStream::StreamState state(
      content::DownloadSourceStream::HAS_DATA);
  size_t incoming_data_size = 0;
  bool should_terminate = false;
  int bytes_to_write = 0;

  // Check if we need to flush any pendind data from previous read calls.
  if (pending_write_buffer_ && pending_write_buffer_->BytesRemaining() > 0) {
    bytes_to_write =
        pending_write_buffer_->BytesRemaining() > current_bytes_remaining_
            ? current_bytes_remaining_
            : pending_write_buffer_->BytesRemaining();
    if (bytes_to_write > 0) {
      memcpy(current_buffer_read_->data() + current_offset_,
             pending_write_buffer_->data(), bytes_to_write);
      current_offset_ += bytes_to_write;
      current_bytes_remaining_ -= bytes_to_write;
      pending_write_buffer_->DidConsume(bytes_to_write);
    }
  }

  while (state == content::DownloadSourceStream::HAS_DATA &&
         !should_terminate && current_bytes_remaining_ > 0) {
    state = stream_->Read(&incoming_data_, &incoming_data_size);
    switch (state) {
      case content::DownloadSourceStream::HAS_DATA: {
        bytes_to_write = incoming_data_size;
        // Check if we need to keep data for the next read call.
        if (bytes_to_write > current_bytes_remaining_) {
          bytes_to_write = current_bytes_remaining_;
          pending_write_buffer_ = new net::DrainableIOBuffer(
              incoming_data_.get(), incoming_data_size);
          pending_write_buffer_->DidConsume(bytes_to_write);
        }
        if (bytes_to_write > 0) {
          memcpy(current_buffer_read_->data() + current_offset_,
                 incoming_data_->data(), bytes_to_write);
          current_offset_ += bytes_to_write;
          current_bytes_remaining_ -= bytes_to_write;
        }
      } break;
      case content::DownloadSourceStream::COMPLETE:
        break;
      case content::DownloadSourceStream::EMPTY:
        break;
      default:
        NOTREACHED();
        break;
    }
  }

  if (state == content::DownloadSourceStream::COMPLETE)
    TransitionTo(COMPLETE);

  if (state == content::DownloadSourceStream::COMPLETE ||
      !current_bytes_remaining_) {
    reading_ = false;
    std::move(callback_).Run(current_offset_);
  }

  if (state == content::DownloadSourceStream::EMPTY && stream_->length() < 0)
    InterruptWithReason(DOWNLOAD_INTERRUPT_REASON_NETWORK_FAILED);
}

void DevToolsDownloadItem::Cancel(bool user_cancel) {
  InterruptWithReason(user_cancel ? DOWNLOAD_INTERRUPT_REASON_USER_CANCELED
                                  : DOWNLOAD_INTERRUPT_REASON_USER_SHUTDOWN);
}

void DevToolsDownloadItem::Remove() {
  InterruptWithReason(DOWNLOAD_INTERRUPT_REASON_USER_CANCELED);
}

const std::string& DevToolsDownloadItem::GetGuid() const {
  return guid_;
}

DownloadItem::DownloadState DevToolsDownloadItem::GetState() const {
  return state_;
}

DownloadInterruptReason DevToolsDownloadItem::GetLastReason() const {
  return last_reason_;
}

const GURL& DevToolsDownloadItem::GetURL() const {
  return info_.url_chain.empty() ? GURL::EmptyGURL() : info_.url_chain.back();
}

const std::vector<GURL>& DevToolsDownloadItem::GetUrlChain() const {
  return info_.url_chain;
}

const GURL& DevToolsDownloadItem::GetOriginalUrl() const {
  return info_.url_chain.empty() ? GURL::EmptyGURL() : info_.url_chain.front();
}

const GURL& DevToolsDownloadItem::GetReferrerUrl() const {
  return info_.referrer_url;
}

const GURL& DevToolsDownloadItem::GetSiteUrl() const {
  return info_.site_url;
}

const GURL& DevToolsDownloadItem::GetTabUrl() const {
  return info_.tab_url;
}

const GURL& DevToolsDownloadItem::GetTabReferrerUrl() const {
  return info_.tab_referrer_url;
}

std::string DevToolsDownloadItem::GetSuggestedFilename() const {
  return GetURL().ExtractFileName();
}

bool DevToolsDownloadItem::IsDone() const {
  return (state_ == COMPLETE || state_ == CANCELLED);
}

const scoped_refptr<const net::HttpResponseHeaders>&
DevToolsDownloadItem::GetResponseHeaders() const {
  return response_headers_;
}

std::string DevToolsDownloadItem::GetContentDisposition() const {
  return content_disposition_;
}

std::string DevToolsDownloadItem::GetMimeType() const {
  return mime_type_;
}

std::string DevToolsDownloadItem::GetOriginalMimeType() const {
  return original_mime_type_;
}

std::string DevToolsDownloadItem::GetRemoteAddress() const {
  return info_.remote_address;
}

bool DevToolsDownloadItem::HasUserGesture() const {
  return info_.has_user_gesture;
}

ui::PageTransition DevToolsDownloadItem::GetTransitionType() const {
  return info_.transition_type;
}

const std::string& DevToolsDownloadItem::GetLastModifiedTime() const {
  return last_modified_time_;
}

const std::string& DevToolsDownloadItem::GetETag() const {
  return etag_;
}

int64_t DevToolsDownloadItem::GetTotalBytes() const {
  return total_bytes_;
}

base::Time DevToolsDownloadItem::GetStartTime() const {
  return info_.start_time;
}

BrowserContext* DevToolsDownloadItem::GetBrowserContext() const {
  DCHECK(request_handle_);
  return request_handle_->GetDownloadManager()->GetBrowserContext();
}

WebContents* DevToolsDownloadItem::GetWebContents() const {
  DCHECK(request_handle_);
  return request_handle_->GetWebContents();
}

bool DevToolsDownloadItem::AllDataSaved() const {
  return IsDone();
}

void DevToolsDownloadItem::SimulateErrorForTesting(
    DownloadInterruptReason reason) {
  InterruptWithReason(reason);
}

uint32_t DevToolsDownloadItem::GetId() const {
  return 0;
}

bool DevToolsDownloadItem::IsPaused() const {
  return false;
}

bool DevToolsDownloadItem::IsTemporary() const {
  return true;
}

bool DevToolsDownloadItem::CanResume() const {
  return false;
}

bool DevToolsDownloadItem::IsSavePackageDownload() const {
  return false;
}

const base::FilePath& DevToolsDownloadItem::GetFullPath() const {
  return empty_path_;
}

const base::FilePath& DevToolsDownloadItem::GetTargetFilePath() const {
  return empty_path_;
}

const base::FilePath& DevToolsDownloadItem::GetForcedFilePath() const {
  return empty_path_;
}

base::FilePath DevToolsDownloadItem::GetFileNameToReportUser() const {
  return empty_path_;
}

DownloadItem::TargetDisposition DevToolsDownloadItem::GetTargetDisposition()
    const {
  return DownloadItem::TARGET_DISPOSITION_OVERWRITE;
}

const std::string& DevToolsDownloadItem::GetHash() const {
  return empty_hash_;
}

bool DevToolsDownloadItem::GetFileExternallyRemoved() const {
  return false;
}

void DevToolsDownloadItem::DeleteFile(
    const base::Callback<void(bool)>& callback) {}

bool DevToolsDownloadItem::IsDangerous() const {
  return false;
}

DownloadDangerType DevToolsDownloadItem::GetDangerType() const {
  return DOWNLOAD_DANGER_TYPE_NOT_DANGEROUS;
}

bool DevToolsDownloadItem::TimeRemaining(base::TimeDelta* remaining) const {
  return false;
}

int64_t DevToolsDownloadItem::CurrentSpeed() const {
  return 0;
}

int DevToolsDownloadItem::PercentComplete() const {
  return -1;
}

int64_t DevToolsDownloadItem::GetReceivedBytes() const {
  return -1;
}

const std::vector<DownloadItem::ReceivedSlice>&
DevToolsDownloadItem::GetReceivedSlices() const {
  return empty_slices_;
}

base::Time DevToolsDownloadItem::GetEndTime() const {
  return base::Time();
}

bool DevToolsDownloadItem::CanShowInFolder() {
  return false;
}

bool DevToolsDownloadItem::CanOpenDownload() {
  return false;
}

bool DevToolsDownloadItem::ShouldOpenFileBasedOnExtension() {
  return false;
}

bool DevToolsDownloadItem::GetOpenWhenComplete() const {
  return false;
}

bool DevToolsDownloadItem::GetAutoOpened() {
  return false;
}

bool DevToolsDownloadItem::GetOpened() const {
  return false;
}

base::Time DevToolsDownloadItem::GetLastAccessTime() const {
  return base::Time();
}

bool DevToolsDownloadItem::IsTransient() const {
  return true;
}

const char* DevToolsDownloadItem::DebugDownloadStateString(
    DownloadItem::DownloadState state) {
  switch (state) {
    case IN_PROGRESS:
      return "IN_PROGRESS";
    case COMPLETE:
      return "COMPLETE";
    case CANCELLED:
      return "CANCELLED";
    case INTERRUPTED:
      return "INTERRUPTED";
    case MAX_DOWNLOAD_STATE:
      break;
  }
  NOTREACHED() << "Unknown download state " << state;
  return "unknown";
}

std::string DevToolsDownloadItem::DebugString(bool verbose) const {
  std::string description = base::StringPrintf(
      "{ id = %s"
      " state = %s }",
      guid_.c_str(), DebugDownloadStateString(state_));

  return description;
}

void DevToolsDownloadItem::TransitionTo(DownloadItem::DownloadState new_state) {
  state_ = new_state;
}

void DevToolsDownloadItem::InterruptWithReason(DownloadInterruptReason reason) {
  DCHECK_NE(DOWNLOAD_INTERRUPT_REASON_NONE, reason);

  last_reason_ = reason;
  TransitionTo(CANCELLED);
}

}  // namespace protocol
}  // namespace content
