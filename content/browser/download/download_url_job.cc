// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// File method ordering: Methods in this file are in the same order as
// in download_item_impl.h, with the following exception: The public
// interface Start is placed in chronological order with the other
// (private) routines that together define a DownloadItem's state
// transitions as the download progresses.  See "Download progression
// cascade" later in this file.

// A regular DownloadItem (created for a download in this session of the
// browser) normally goes through the following states:
//      * Created (when download starts)
//      * Destination filename determined
//      * Entered into the history database.
//      * Made visible in the download shelf.
//      * All the data is saved.  Note that the actual data download occurs
//        in parallel with the above steps, but until those steps are
//        complete, the state of the data save will be ignored.
//      * Download file is renamed to its final name, and possibly
//        auto-opened.

#include "content/browser/download/download_url_job.h"

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
#include "content/browser/download/download_create_info.h"
#include "content/browser/download/download_file.h"
#include "content/browser/download/download_interrupt_reasons_impl.h"
#include "content/browser/download/download_net_log_parameters.h"
#include "content/browser/download/download_request_handle.h"
#include "content/browser/download/download_stats.h"
#include "content/browser/download/download_url_job_delegate.h"
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
#include "net/log/net_log.h"
#include "net/log/net_log_event_type.h"
#include "net/log/net_log_parameters_callback.h"
#include "net/log/net_log_source.h"

namespace content {

namespace {

bool DeleteDownloadedFile(const base::FilePath& path) {
  DCHECK_CURRENTLY_ON(BrowserThread::FILE);

  // Make sure we only delete files.
  if (base::DirectoryExists(path))
    return true;
  return base::DeleteFile(path, false);
}

void DeleteDownloadedFileDone(base::WeakPtr<DownloadUrlJob> item,
                              const base::Callback<void(bool)>& callback,
                              bool success) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (success && item.get())
    item->OnDownloadedFileRemoved();
  callback.Run(success);
}

// Wrapper around DownloadFile::Detach and DownloadFile::Cancel that
// takes ownership of the DownloadFile and hence implicitly destroys it
// at the end of the function.
static base::FilePath DownloadFileDetach(
    std::unique_ptr<DownloadFile> download_file) {
  DCHECK_CURRENTLY_ON(BrowserThread::FILE);
  base::FilePath full_path = download_file->FullPath();
  download_file->Detach();
  return full_path;
}

static base::FilePath MakeCopyOfDownloadFile(DownloadFile* download_file) {
  DCHECK_CURRENTLY_ON(BrowserThread::FILE);
  base::FilePath temp_file_path;
  if (base::CreateTemporaryFile(&temp_file_path) &&
      base::CopyFile(download_file->FullPath(), temp_file_path)) {
    return temp_file_path;
  } else {
    // Deletes the file at |temp_file_path|.
    if (!base::DirectoryExists(temp_file_path))
      base::DeleteFile(temp_file_path, false);
    temp_file_path.clear();
    return base::FilePath();
  }
}

static void DownloadFileCancel(std::unique_ptr<DownloadFile> download_file) {
  DCHECK_CURRENTLY_ON(BrowserThread::FILE);
  download_file->Cancel();
}

}  // namespace

// The maximum number of attempts we will make to resume automatically.
const int DownloadUrlJob::kMaxAutoResumeAttempts = 5;

// Constructing for a regular download:
DownloadUrlJob::DownloadUrlJob(
    DownloadUrlJobDelegate* delegate,
    const DownloadSaveInfo& save_info,
    std::unique_ptr<DownloadItem::ResponseInfo> response_info,
    std::unique_ptr<DownloadRequestHandleInterface> request_handle,
    const net::NetLogWithSource& request_bound_net_log,
    const net::NetLogWithSource& download_bound_net_log)
    : DownloadJob(),
      target_disposition_((save_info.prompt_for_save_location)
                              ? DownloadItem::TARGET_DISPOSITION_PROMPT
                              : DownloadItem::TARGET_DISPOSITION_OVERWRITE),
      request_handle_(std::move(request_handle)),
      initial_response_info_(std::move(response_info)),
      start_tick_(base::TimeTicks::Now()),
      state_(INITIAL_INTERNAL),
      delegate_(delegate),
      net_log_(download_bound_net_log),
      weak_ptr_factory_(this) {
  // Link the event sources.
  net_log_.AddEvent(
      net::NetLogEventType::DOWNLOAD_URL_REQUEST,
      request_bound_net_log.source().ToEventParametersCallback());

  request_bound_net_log.AddEvent(
      net::NetLogEventType::DOWNLOAD_STARTED,
      net_log_.source().ToEventParametersCallback());

  delegate_->Attach();
  Init(true /* actively downloading */);
}

DownloadUrlJob::~DownloadUrlJob() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  // Should always have been nuked before now, at worst in
  // DownloadManager shutdown.
  DCHECK(!download_file_.get());

  delegate_->AssertStateConsistent(this);
  delegate_->Detach();
}

void DownloadUrlJob::ValidateDangerousDownload() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(!IsDone());
  DCHECK(IsDangerous());

  DVLOG(20) << __func__ << "() download=" << DebugString(true);

  if (IsDone() || !IsDangerous())
    return;

  RecordDangerousDownloadAccept(GetDangerType(), GetTargetFilePath());

  danger_type_ = DOWNLOAD_DANGER_TYPE_USER_VALIDATED;

  net_log_.AddEvent(
      net::NetLogEventType::DOWNLOAD_ITEM_SAFETY_STATE_UPDATED,
      base::Bind(&ItemCheckedNetLogCallback, GetDangerType()));

  UpdateObservers();  // TODO(asanka): This is potentially unsafe. The download
                      // may not be in a consistent state or around at all after
                      // invoking observers. http://crbug.com/586610

  MaybeCompleteDownload();
}

void DownloadUrlJob::StealDangerousDownload(
    bool delete_file_afterward,
    const AcquireFileCallback& callback) {
  DVLOG(20) << __func__ << "() download = " << DebugString(true);
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(IsDangerous());
  DCHECK(AllDataSaved());

  if (delete_file_afterward) {
    if (download_file_) {
      BrowserThread::PostTaskAndReplyWithResult(
          BrowserThread::FILE, FROM_HERE,
          base::Bind(&DownloadFileDetach, base::Passed(&download_file_)),
          callback);
    } else {
      callback.Run(current_path_);
    }
    current_path_.clear();
    Remove();
    // Download item has now been deleted.
  } else if (download_file_) {
    BrowserThread::PostTaskAndReplyWithResult(
        BrowserThread::FILE, FROM_HERE,
        base::Bind(&MakeCopyOfDownloadFile, download_file_.get()), callback);
  } else {
    callback.Run(current_path_);
  }
}

void DownloadUrlJob::Pause() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  // Ignore irrelevant states.
  if (is_paused_)
    return;

  switch (state_) {
    case CANCELLED_INTERNAL:
    case COMPLETE_INTERNAL:
    case COMPLETING_INTERNAL:
    case INITIAL_INTERNAL:
    case INTERRUPTED_INTERNAL:
    case INTERRUPTED_TARGET_PENDING_INTERNAL:
    case RESUMING_INTERNAL:
      // No active request.
      // TODO(asanka): In the case of RESUMING_INTERNAL, consider setting
      // is_paused_ even if there's no request currently associated with this
      // DII. When a request is assigned (due to a resumption, for example) we
      // can honor the is_paused_ setting.
      return;

    case IN_PROGRESS_INTERNAL:
    case TARGET_PENDING_INTERNAL:
      request_handle_->PauseRequest();
      is_paused_ = true;
      UpdateObservers();
      return;

    case MAX_DOWNLOAD_INTERNAL_STATE:
    case TARGET_RESOLVED_INTERNAL:
      NOTREACHED();
  }
}

void DownloadUrlJob::Resume() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DVLOG(20) << __func__ << "() download = " << DebugString(true);
  switch (state_) {
    case CANCELLED_INTERNAL:  // Nothing to resume.
    case COMPLETE_INTERNAL:
    case COMPLETING_INTERNAL:
    case INITIAL_INTERNAL:
    case INTERRUPTED_TARGET_PENDING_INTERNAL:
    case RESUMING_INTERNAL:  // Resumption in progress.
      return;

    case TARGET_PENDING_INTERNAL:
    case IN_PROGRESS_INTERNAL:
      if (!is_paused_)
        return;
      request_handle_->ResumeRequest();
      is_paused_ = false;
      UpdateObservers();
      return;

    case INTERRUPTED_INTERNAL:
      auto_resume_count_ = 0;  // User input resets the counter.
      ResumeInterruptedDownload(ResumptionRequestSource::USER);
      UpdateObservers();
      return;

    case MAX_DOWNLOAD_INTERNAL_STATE:
    case TARGET_RESOLVED_INTERNAL:
      NOTREACHED();
  }
}

void DownloadUrlJob::Cancel(bool user_cancel) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DVLOG(20) << __func__ << "() download = " << DebugString(true);
  InterruptAndDiscardPartialState(
      user_cancel ? DOWNLOAD_INTERRUPT_REASON_USER_CANCELED
                  : DOWNLOAD_INTERRUPT_REASON_USER_SHUTDOWN);
  UpdateObservers();
}

void DownloadUrlJob::Remove() {
  DVLOG(20) << __func__ << "() download = " << DebugString(true);
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  delegate_->AssertStateConsistent(this);
  InterruptAndDiscardPartialState(DOWNLOAD_INTERRUPT_REASON_USER_CANCELED);
  UpdateObservers();
  delegate_->AssertStateConsistent(this);

  NotifyRemoved();
  delegate_->DownloadRemoved(this);
  // We have now been deleted.
}

void DownloadUrlJob::OpenDownload() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  if (!IsDone()) {
    // We don't honor the open_when_complete_ flag for temporary
    // downloads. Don't set it because it shows up in the UI.
    if (!IsTemporary())
      open_when_complete_ = !open_when_complete_;
    return;
  }

  if (state_ != COMPLETE_INTERNAL || file_externally_removed_)
    return;

  // Ideally, we want to detect errors in opening and report them, but we
  // don't generally have the proper interface for that to the external
  // program that opens the file.  So instead we spawn a check to update
  // the UI if the file has been deleted in parallel with the open.
  delegate_->CheckForFileRemoval(this);
  RecordOpen(GetEndTime(), !GetOpened());
  opened_ = true;
  for (auto& observer : observers_)
    observer.OnDownloadOpened(this);
  delegate_->OpenDownload(this);
}

void DownloadUrlJob::ShowDownloadInShell() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  delegate_->ShowDownloadInShell(this);
}

DownloadItem::DownloadState DownloadUrlJob::GetState() const {
  return InternalToExternalState(state_);
}

bool DownloadUrlJob::CanResume() const {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  switch (state_) {
    case INITIAL_INTERNAL:
    case COMPLETING_INTERNAL:
    case COMPLETE_INTERNAL:
    case CANCELLED_INTERNAL:
    case RESUMING_INTERNAL:
    case INTERRUPTED_TARGET_PENDING_INTERNAL:
      return false;

    case TARGET_PENDING_INTERNAL:
    case TARGET_RESOLVED_INTERNAL:
    case IN_PROGRESS_INTERNAL:
      return is_paused_;

    case INTERRUPTED_INTERNAL: {
      ResumeMode resume_mode = GetResumeMode();
      // Only allow Resume() calls if the resumption mode requires a user
      // action.
      return resume_mode == RESUME_MODE_USER_RESTART ||
             resume_mode == RESUME_MODE_USER_CONTINUE;
    }

    case MAX_DOWNLOAD_INTERNAL_STATE:
      NOTREACHED();
  }
  return false;
}

bool DownloadUrlJob::IsDone() const {
  switch (state_) {
    case INITIAL_INTERNAL:
    case COMPLETING_INTERNAL:
    case RESUMING_INTERNAL:
    case TARGET_PENDING_INTERNAL:
    case INTERRUPTED_TARGET_PENDING_INTERNAL:
    case TARGET_RESOLVED_INTERNAL:
    case IN_PROGRESS_INTERNAL:
      return false;

    case COMPLETE_INTERNAL:
    case CANCELLED_INTERNAL:
      return true;

    case INTERRUPTED_INTERNAL:
      return !CanResume();

    case MAX_DOWNLOAD_INTERNAL_STATE:
      NOTREACHED();
  }
  return false;
}

void DownloadUrlJob::DeleteFile(const base::Callback<void(bool)>& callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (GetState() != DownloadItem::COMPLETE) {
    // Pass a null WeakPtr so it doesn't call OnDownloadedFileRemoved.
    BrowserThread::PostTask(
        BrowserThread::UI, FROM_HERE,
        base::Bind(&DeleteDownloadedFileDone, base::WeakPtr<DownloadUrlJob>(),
                   callback, false));
    return;
  }
  if (current_path_.empty() || file_externally_removed_) {
    // Pass a null WeakPtr so it doesn't call OnDownloadedFileRemoved.
    BrowserThread::PostTask(
        BrowserThread::UI, FROM_HERE,
        base::Bind(&DeleteDownloadedFileDone, base::WeakPtr<DownloadUrlJob>(),
                   callback, true));
    return;
  }
  BrowserThread::PostTaskAndReplyWithResult(
      BrowserThread::FILE, FROM_HERE,
      base::Bind(&DeleteDownloadedFile, current_path_),
      base::Bind(&DeleteDownloadedFileDone, weak_ptr_factory_.GetWeakPtr(),
                 callback));
}

bool DownloadUrlJob::TimeRemaining(base::TimeDelta* remaining) const {
  if (total_bytes_ <= 0)
    return false;  // We never received the content_length for this download.

  int64_t speed = CurrentSpeed();
  if (speed == 0)
    return false;

  *remaining =
      base::TimeDelta::FromSeconds((total_bytes_ - received_bytes_) / speed);
  return true;
}

int64_t DownloadUrlJob::CurrentSpeed() const {
  if (is_paused_)
    return 0;
  return bytes_per_sec_;
}

int DownloadUrlJob::PercentComplete() const {
  // If the delegate is delaying completion of the download, then we have no
  // idea how long it will take.
  if (delegate_delayed_complete_ || total_bytes_ <= 0)
    return -1;

  return static_cast<int>(received_bytes_ * 100.0 / total_bytes_);
}

bool DownloadUrlJob::CanShowInFolder() {
  // A download can be shown in the folder if the downloaded file is in a known
  // location.
  return CanOpenDownload() && !GetFullPath().empty();
}

bool DownloadUrlJob::CanOpenDownload() {
  // We can open the file or mark it for opening on completion if the download
  // is expected to complete successfully. Exclude temporary downloads, since
  // they aren't owned by the download system.
  const bool is_complete = GetState() == DownloadItem::COMPLETE;
  return (!IsDone() || is_complete) && !IsTemporary() &&
         !file_externally_removed_;
}

bool DownloadUrlJob::ShouldOpenFileBasedOnExtension() {
  return delegate_->ShouldOpenFileBasedOnExtension(GetTargetFilePath());
}

BrowserContext* DownloadUrlJob::GetBrowserContext() const {
  return delegate_->GetBrowserContext();
}

WebContents* DownloadUrlJob::GetWebContents() const {
  // TODO(rdsmith): Remove null check after removing GetWebContents() from
  // paths that might be used by DownloadItems created from history import.
  // Currently such items have null request_handle_s, where other items
  // (regular and SavePackage downloads) have actual objects off the pointer.
  if (request_handle_)
    return request_handle_->GetWebContents();
  return NULL;
}

std::string DownloadUrlJob::DebugString(bool verbose) const {
  std::string description = base::StringPrintf(
      "{ id = %d"
      " state = %s",
      download_id_, DebugDownloadStateString(state_));

  // Construct a string of the URL chain.
  std::string url_list("<none>");
  if (!url_chain_.empty()) {
    std::vector<GURL>::const_iterator iter = url_chain_.begin();
    std::vector<GURL>::const_iterator last = url_chain_.end();
    url_list = (*iter).is_valid() ? (*iter).spec() : "<invalid>";
    ++iter;
    for (; verbose && (iter != last); ++iter) {
      url_list += " ->\n\t";
      const GURL& next_url = *iter;
      url_list += next_url.is_valid() ? next_url.spec() : "<invalid>";
    }
  }

  if (verbose) {
    description += base::StringPrintf(
        " total = %" PRId64 " received = %" PRId64
        " reason = %s"
        " paused = %c"
        " resume_mode = %s"
        " auto_resume_count = %d"
        " danger = %d"
        " all_data_saved = %c"
        " last_modified = '%s'"
        " etag = '%s'"
        " has_download_file = %s"
        " url_chain = \n\t\"%s\"\n\t"
        " current_path = \"%" PRFilePath
        "\"\n\t"
        " target_path = \"%" PRFilePath
        "\""
        " referrer = \"%s\""
        " site_url = \"%s\"",
        GetTotalBytes(), GetReceivedBytes(),
        DownloadInterruptReasonToString(last_reason_).c_str(),
        IsPaused() ? 'T' : 'F', DebugResumeModeString(GetResumeMode()),
        auto_resume_count_, GetDangerType(), AllDataSaved() ? 'T' : 'F',
        GetLastModifiedTime().c_str(), GetETag().c_str(),
        download_file_.get() ? "true" : "false", url_list.c_str(),
        GetFullPath().value().c_str(), GetTargetFilePath().value().c_str(),
        GetReferrerUrl().spec().c_str(), GetSiteUrl().spec().c_str());
  } else {
    description += base::StringPrintf(" url = \"%s\"", url_list.c_str());
  }

  description += " }";

  return description;
}

DownloadUrlJob::ResumeMode DownloadUrlJob::GetResumeMode() const {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  // Only support resumption for HTTP(S).
  if (!GetURL().SchemeIsHTTPOrHTTPS())
    return RESUME_MODE_INVALID;

  // We can't continue without a handle on the intermediate file.
  // We also can't continue if we don't have some verifier to make sure
  // we're getting the same file.
  bool restart_required =
      (current_path_.empty() || (etag_.empty() && last_modified_time_.empty()));

  // We won't auto-restart if we've used up our attempts or the
  // download has been paused by user action.
  bool user_action_required =
      (auto_resume_count_ >= kMaxAutoResumeAttempts || is_paused_);

  switch (last_reason_) {
    case DOWNLOAD_INTERRUPT_REASON_FILE_TRANSIENT_ERROR:
    case DOWNLOAD_INTERRUPT_REASON_NETWORK_TIMEOUT:
      break;

    case DOWNLOAD_INTERRUPT_REASON_SERVER_NO_RANGE:
    // The server disagreed with the file offset that we sent.

    case DOWNLOAD_INTERRUPT_REASON_FILE_HASH_MISMATCH:
    // The file on disk was found to not match the expected hash. Discard and
    // start from beginning.

    case DOWNLOAD_INTERRUPT_REASON_FILE_TOO_SHORT:
      // The [possibly persisted] file offset disagreed with the file on disk.

      // The intermediate stub is not usable and the server is responding. Hence
      // retrying the request from the beginning is likely to work.
      restart_required = true;
      break;

    case DOWNLOAD_INTERRUPT_REASON_NETWORK_FAILED:
    case DOWNLOAD_INTERRUPT_REASON_NETWORK_DISCONNECTED:
    case DOWNLOAD_INTERRUPT_REASON_NETWORK_SERVER_DOWN:
    case DOWNLOAD_INTERRUPT_REASON_SERVER_FAILED:
    case DOWNLOAD_INTERRUPT_REASON_SERVER_UNREACHABLE:
    case DOWNLOAD_INTERRUPT_REASON_USER_SHUTDOWN:
    case DOWNLOAD_INTERRUPT_REASON_CRASH:
      // It is not clear whether attempting a resumption is acceptable at this
      // time or whether it would work at all. Hence allow the user to retry the
      // download manually.
      user_action_required = true;
      break;

    case DOWNLOAD_INTERRUPT_REASON_FILE_NO_SPACE:
      // There was no space. Require user interaction so that the user may, for
      // example, choose a different location to store the file. Or they may
      // free up some space on the targret device and retry. But try to reuse
      // the partial stub.
      user_action_required = true;
      break;

    case DOWNLOAD_INTERRUPT_REASON_FILE_FAILED:
    case DOWNLOAD_INTERRUPT_REASON_FILE_ACCESS_DENIED:
    case DOWNLOAD_INTERRUPT_REASON_FILE_NAME_TOO_LONG:
    case DOWNLOAD_INTERRUPT_REASON_FILE_TOO_LARGE:
      // Assume the partial stub is unusable. Also it may not be possible to
      // restart immediately.
      user_action_required = true;
      restart_required = true;
      break;

    case DOWNLOAD_INTERRUPT_REASON_NONE:
    case DOWNLOAD_INTERRUPT_REASON_NETWORK_INVALID_REQUEST:
    case DOWNLOAD_INTERRUPT_REASON_FILE_VIRUS_INFECTED:
    case DOWNLOAD_INTERRUPT_REASON_SERVER_BAD_CONTENT:
    case DOWNLOAD_INTERRUPT_REASON_USER_CANCELED:
    case DOWNLOAD_INTERRUPT_REASON_FILE_BLOCKED:
    case DOWNLOAD_INTERRUPT_REASON_FILE_SECURITY_CHECK_FAILED:
    case DOWNLOAD_INTERRUPT_REASON_SERVER_UNAUTHORIZED:
    case DOWNLOAD_INTERRUPT_REASON_SERVER_CERT_PROBLEM:
    case DOWNLOAD_INTERRUPT_REASON_SERVER_FORBIDDEN:
      // Unhandled.
      return RESUME_MODE_INVALID;
  }

  if (user_action_required && restart_required)
    return RESUME_MODE_USER_RESTART;

  if (restart_required)
    return RESUME_MODE_IMMEDIATE_RESTART;

  if (user_action_required)
    return RESUME_MODE_USER_CONTINUE;

  return RESUME_MODE_IMMEDIATE_CONTINUE;
}

void DownloadUrlJob::UpdateValidatorsOnResumption(
    const DownloadCreateInfo& new_create_info) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK_EQ(RESUMING_INTERNAL, state_);
  DCHECK(!new_create_info.url_chain.empty());

  // We are going to tack on any new redirects to our list of redirects.
  // When a download is resumed, the URL used for the resumption request is the
  // one at the end of the previous redirect chain. Tacking additional redirects
  // to the end of this chain ensures that:
  // - If the download needs to be resumed again, the ETag/Last-Modified headers
  //   will be used with the last server that sent them to us.
  // - The redirect chain contains all the servers that were involved in this
  //   download since the initial request, in order.
  std::vector<GURL>::const_iterator chain_iter =
      new_create_info.url_chain.begin();
  if (*chain_iter == url_chain_.back())
    ++chain_iter;

  // Record some stats. If the precondition failed (the server returned
  // HTTP_PRECONDITION_FAILED), then the download will automatically retried as
  // a full request rather than a partial. Full restarts clobber validators.
  int origin_state = 0;
  if (chain_iter != new_create_info.url_chain.end())
    origin_state |= ORIGIN_STATE_ON_RESUMPTION_ADDITIONAL_REDIRECTS;
  if (etag_ != new_create_info.etag ||
      last_modified_time_ != new_create_info.last_modified)
    origin_state |= ORIGIN_STATE_ON_RESUMPTION_VALIDATORS_CHANGED;
  if (content_disposition_ != new_create_info.content_disposition)
    origin_state |= ORIGIN_STATE_ON_RESUMPTION_CONTENT_DISPOSITION_CHANGED;
  RecordOriginStateOnResumption(received_bytes_ != 0, origin_state);

  url_chain_.insert(url_chain_.end(), chain_iter,
                    new_create_info.url_chain.end());
  etag_ = new_create_info.etag;
  last_modified_time_ = new_create_info.last_modified;
  content_disposition_ = new_create_info.content_disposition;
  // It is possible that the previous download attempt failed right before the
  // response is received. Need to reset the MIME type.
  mime_type_ = new_create_info.mime_type;

  // Don't update observers. This method is expected to be called just before a
  // DownloadFile is created and Start() is called. The observers will be
  // notified when the download transitions to the IN_PROGRESS state.
}

void DownloadUrlJob::NotifyRemoved() {
  for (auto& observer : observers_)
    observer.OnDownloadRemoved(this);
}

void DownloadUrlJob::OnDownloadedFileRemoved() {
  file_externally_removed_ = true;
  DVLOG(20) << __func__ << "() download=" << DebugString(true);
  UpdateObservers();
}

base::WeakPtr<DownloadDestinationObserver>
DownloadUrlJob::DestinationObserverAsWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

const net::NetLogWithSource& DownloadItemImpl::GetNetLogWithSource() const {
  return net_log_;
}

void DownloadUrlJob::OnAllDataSaved(
    int64_t total_bytes,
    std::unique_ptr<crypto::SecureHash> hash_state) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(!all_data_saved_);
  all_data_saved_ = true;
  SetTotalBytes(total_bytes);
  UpdateProgress(total_bytes, 0);
  SetHashState(std::move(hash_state));
  hash_state_.reset();  // No need to retain hash_state_ since we are done with
                        // the download and don't expect to receive any more
                        // data.

  DVLOG(20) << __func__ << "() download=" << DebugString(true);
  UpdateObservers();
}

void DownloadUrlJob::MarkAsComplete() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  DCHECK(all_data_saved_);
  end_time_ = base::Time::Now();
  TransitionTo(COMPLETE_INTERNAL);
  UpdateObservers();
}

void DownloadUrlJob::DestinationUpdate(int64_t bytes_so_far,
                                       int64_t bytes_per_sec) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  // If the download is in any other state we don't expect any
  // DownloadDestinationObserver callbacks. An interruption or a cancellation
  // results in a call to ReleaseDownloadFile which invalidates the weak
  // reference held by the DownloadFile and hence cuts off any pending
  // callbacks.
  DCHECK(state_ == TARGET_PENDING_INTERNAL || state_ == IN_PROGRESS_INTERNAL);

  // There must be no pending destination_error_.
  DCHECK_EQ(destination_error_, DOWNLOAD_INTERRUPT_REASON_NONE);

  DVLOG(20) << __func__ << "() so_far=" << bytes_so_far
            << " per_sec=" << bytes_per_sec
            << " download=" << DebugString(true);

  UpdateProgress(bytes_so_far, bytes_per_sec);
  if (net_log_.IsCapturing()) {
    net_log_.AddEvent(
        net::NetLogEventType::DOWNLOAD_ITEM_UPDATED,
        net::NetLog::Int64Callback("bytes_so_far", received_bytes_));
  }

  UpdateObservers();
}

void DownloadUrlJob::DestinationError(
    DownloadInterruptReason reason,
    int64_t bytes_so_far,
    std::unique_ptr<crypto::SecureHash> secure_hash) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  // If the download is in any other state we don't expect any
  // DownloadDestinationObserver callbacks. An interruption or a cancellation
  // results in a call to ReleaseDownloadFile which invalidates the weak
  // reference held by the DownloadFile and hence cuts off any pending
  // callbacks.
  DCHECK(state_ == TARGET_PENDING_INTERNAL || state_ == IN_PROGRESS_INTERNAL);
  DVLOG(20) << __func__
            << "() reason:" << DownloadInterruptReasonToString(reason);

  // Postpone recognition of this error until after file name determination
  // has completed and the intermediate file has been renamed to simplify
  // resumption conditions.
  if (state_ == TARGET_PENDING_INTERNAL) {
    received_bytes_ = bytes_so_far;
    hash_state_ = std::move(secure_hash);
    hash_.clear();
    destination_error_ = reason;
    return;
  }
  InterruptWithPartialState(bytes_so_far, std::move(secure_hash), reason);
  UpdateObservers();
}

void DownloadUrlJob::DestinationCompleted(
    int64_t total_bytes,
    std::unique_ptr<crypto::SecureHash> secure_hash) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  // If the download is in any other state we don't expect any
  // DownloadDestinationObserver callbacks. An interruption or a cancellation
  // results in a call to ReleaseDownloadFile which invalidates the weak
  // reference held by the DownloadFile and hence cuts off any pending
  // callbacks.
  DCHECK(state_ == TARGET_PENDING_INTERNAL || state_ == IN_PROGRESS_INTERNAL);
  DVLOG(20) << __func__ << "() download=" << DebugString(true);

  OnAllDataSaved(total_bytes, std::move(secure_hash));
  MaybeCompleteDownload();
}

// **** Download progression cascade

void DownloadUrlJob::Init(bool active) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  if (active)
    RecordDownloadCount(START_COUNT);

  std::string file_name;
  if (download_type == SRC_HISTORY_IMPORT) {
    // target_path_ works for History and Save As versions.
    file_name = target_path_.AsUTF8Unsafe();
  } else {
    // See if it's set programmatically.
    file_name = forced_file_path_.AsUTF8Unsafe();
    // Possibly has a 'download' attribute for the anchor.
    if (file_name.empty())
      file_name = suggested_filename_;
    // From the URL file name.
    if (file_name.empty())
      file_name = GetURL().ExtractFileName();
  }

  net::NetLogParametersCallback active_data =
      base::Bind(&ItemActivatedNetLogCallback, this, download_type, &file_name);
  if (active) {
    net_log_.BeginEvent(net::NetLogEventType::DOWNLOAD_ITEM_ACTIVE,
                              active_data);
  } else {
    net_log_.AddEvent(net::NetLogEventType::DOWNLOAD_ITEM_ACTIVE,
                            active_data);
  }

  DVLOG(20) << __func__ << "() " << DebugString(true);
}

void DownloadUrlJob::SetDownloadFile(std::unique_ptr<DownloadFile> file) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(!download_file_.get());
  download_file_ = std::move(file);
}

void DownloadUrlJob::SetInitialIterruptedState(
    DownloadInterruptReason reason,
    std::unique_ptr<DownloadSaveInfo> interrupted_save_info) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(!download_file_.get());
  pending_interrupt_reason_ = reason;
  interrupted_save_info_ = std::move(interrupted_save_info);
}

void DownloadUrlJob::OnBeforeDetach() {}

void DownloadUrlJob::OnAttached() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(!download_file_.get());
  DVLOG(20) << __func__ << "() this=" << DebugString(true);

  // TODO(asanka): This can also be called for INTERRUPTED downloads that were
  // restored from history.

  destination_info().target_disposition = target_disposition_;
  destination_error_ = DOWNLOAD_INTERRUPT_REASON_NONE;

  // The state could be one of the following:
  //
  // INITIAL_INTERNAL: A normal download attempt.
  //
  // RESUMING_INTERNAL: A resumption attempt. May or may not have been
  //     successful.
  DCHECK(state_ == INITIAL_INTERNAL || state_ == RESUMING_INTERNAL);

  // If the state_ is INITIAL_INTERNAL, then the target path must be empty.
  DCHECK(state_ != INITIAL_INTERNAL || target_path_.empty());

  // If a resumption attempted failed, or if the download was DOA, then the
  // download should go back to being interrupted.
  if (pending_interrupt_reason_ != DOWNLOAD_INTERRUPT_REASON_NONE) {
    // Download requests that are interrupted by Start() should result in a
    // DownloadCreateInfo with an intact DownloadSaveInfo.
    DCHECK(interrupted_save_info_);

    int64_t offset = interrupted_save_info_->offset;
    std::unique_ptr<crypto::SecureHash> hash_state =
        interrupted_save_info_->hash_state
            ? interrupted_save_info_->hash_state->Clone()
            : nullptr;

    // Interrupted downloads also need a target path.
    if (target_path_.empty()) {
      received_bytes_ = offset;
      hash_state_ = std::move(hash_state);
      hash_.clear();
      destination_error_ = pending_interrupt_reason_;
      TransitionTo(INTERRUPTED_TARGET_PENDING_INTERNAL);
      DetermineDownloadTarget();
      pending_interrupt_reason_ = DOWNLOAD_INTERRUPT_REASON_NONE;
      return;
    }

    // Otherwise, this was a resumption attempt which ended with an
    // interruption. Continue with current target path.
    TransitionTo(TARGET_RESOLVED_INTERNAL);
    InterruptWithPartialState(offset, std::move(hash_state),
                              pending_interrupt_reason_);
    UpdateObservers();
    pending_interrupt_reason_ = DOWNLOAD_INTERRUPT_REASON_NONE;
    return;
  }

  // Successful download start.
  DCHECK(download_file_.get());
  DCHECK(request_handle_.get());

  if (state_ == RESUMING_INTERNAL)
    UpdateValidatorsOnResumption();

  TransitionTo(TARGET_PENDING_INTERNAL);
  StartedSavingResponse(*initial_response_info_);
  initial_response_info_.reset();

  BrowserThread::PostTask(
      BrowserThread::FILE, FROM_HERE,
      base::Bind(&DownloadFile::Initialize,
                 // Safe because we control download file lifetime.
                 base::Unretained(download_file_.get()),
                 base::Bind(&DownloadUrlJob::OnDownloadFileInitialized,
                            weak_ptr_factory_.GetWeakPtr())));
}

void DownloadUrlJob::OnDownloadFileInitialized(DownloadInterruptReason result) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK_EQ(state_, TARGET_PENDING_INTERNAL);
  DVLOG(20) << __func__
            << "() result:" << DownloadInterruptReasonToString(result);
  if (result != DOWNLOAD_INTERRUPT_REASON_NONE) {
    // Whoops. That didn't work. Proceed as an interrupted download, but reset
    // the partial state. Currently, the partial stub cannot be recovered if the
    // download file initialization fails.
    received_bytes_ = 0;
    hash_state_.reset();
    hash_.clear();
    destination_error_ = result;
    TransitionTo(INTERRUPTED_TARGET_PENDING_INTERNAL);
  }

  DetermineDownloadTarget();
}

void DownloadUrlJob::DetermineDownloadTarget() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DVLOG(20) << __func__ << "() " << DebugString(true);

  delegate_->DetermineDownloadTarget(
      this, base::Bind(&DownloadUrlJob::OnDownloadTargetDetermined,
                       weak_ptr_factory_.GetWeakPtr()));
}

// Called by delegate_ when the download target path has been determined.
void DownloadUrlJob::OnDownloadTargetDetermined(
    const base::FilePath& target_path,
    TargetDisposition disposition,
    DownloadDangerType danger_type,
    const base::FilePath& intermediate_path) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(state_ == TARGET_PENDING_INTERNAL ||
         state_ == INTERRUPTED_TARGET_PENDING_INTERNAL);

  // If the |target_path| is empty, then we consider this download to be
  // canceled.
  if (target_path.empty()) {
    Cancel(true);
    return;
  }

  DVLOG(20) << __func__ << "() target_path:" << target_path.value()
            << " disposition:" << disposition << " danger_type:" << danger_type
            << " this:" << DebugString(true);

  target_path_ = target_path;
  target_disposition_ = disposition;
  SetDangerType(danger_type);

  // This was an interrupted download that was looking for a filename. Now that
  // it has one, transition to interrupted.
  if (state_ == INTERRUPTED_TARGET_PENDING_INTERNAL) {
    InterruptWithPartialState(received_bytes_, std::move(hash_state_),
                              destination_error_);
    destination_error_ = DOWNLOAD_INTERRUPT_REASON_NONE;
    UpdateObservers();
    return;
  }

  // We want the intermediate and target paths to refer to the same directory so
  // that they are both on the same device and subject to same
  // space/permission/availability constraints.
  DCHECK(intermediate_path.DirName() == target_path.DirName());

  // During resumption, we may choose to proceed with the same intermediate
  // file. No rename is necessary if our intermediate file already has the
  // correct name.
  //
  // The intermediate name may change from its original value during filename
  // determination on resumption, for example if the reason for the interruption
  // was the download target running out space, resulting in a user prompt.
  if (intermediate_path == current_path_) {
    OnDownloadRenamedToIntermediateName(DOWNLOAD_INTERRUPT_REASON_NONE,
                                        intermediate_path);
    return;
  }

  // Rename to intermediate name.
  // TODO(asanka): Skip this rename if AllDataSaved() is true. This avoids a
  //               spurious rename when we can just rename to the final
  //               filename. Unnecessary renames may cause bugs like
  //               http://crbug.com/74187.
  DCHECK(!is_save_package_download_);
  DCHECK(download_file_.get());
  DownloadFile::RenameCompletionCallback callback =
      base::Bind(&DownloadUrlJob::OnDownloadRenamedToIntermediateName,
                 weak_ptr_factory_.GetWeakPtr());
  BrowserThread::PostTask(
      BrowserThread::FILE, FROM_HERE,
      base::Bind(&DownloadFile::RenameAndUniquify,
                 // Safe because we control download file lifetime.
                 base::Unretained(download_file_.get()), intermediate_path,
                 callback));
}

void DownloadUrlJob::OnDownloadRenamedToIntermediateName(
    DownloadInterruptReason reason,
    const base::FilePath& full_path) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK_EQ(state_, TARGET_PENDING_INTERNAL);
  DVLOG(20) << __func__ << "() download=" << DebugString(true);

  TransitionTo(TARGET_RESOLVED_INTERNAL);

  // If the intermediate rename fails while there's also a destination_error_,
  // then the former is considered the critical error since it requires
  // discarding the partial state.
  if (DOWNLOAD_INTERRUPT_REASON_NONE != reason) {
    // TODO(asanka): Even though the rename failed, it may still be possible to
    // recover the partial state from the 'before' name.
    InterruptAndDiscardPartialState(reason);
    UpdateObservers();
    return;
  }

  if (DOWNLOAD_INTERRUPT_REASON_NONE != destination_error_) {
    SetFullPath(full_path);
    InterruptWithPartialState(received_bytes_, std::move(hash_state_),
                              destination_error_);
    destination_error_ = DOWNLOAD_INTERRUPT_REASON_NONE;
    UpdateObservers();
    return;
  }

  SetFullPath(full_path);
  TransitionTo(IN_PROGRESS_INTERNAL);
  // TODO(asanka): Calling UpdateObservers() prior to MaybeCompleteDownload() is
  // not safe. The download could be in an underminate state after invoking
  // observers. http://crbug.com/586610
  UpdateObservers();
  MaybeCompleteDownload();
}

// When SavePackage downloads MHTML to GData (see
// SavePackageFilePickerChromeOS), GData calls MaybeCompleteDownload() like it
// does for non-SavePackage downloads, but SavePackage downloads never satisfy
// IsDownloadReadyForCompletion(). GDataDownloadObserver manually calls
// DownloadItem::UpdateObservers() when the upload completes so that SavePackage
// notices that the upload has completed and runs its normal Finish() pathway.
// MaybeCompleteDownload() is never the mechanism by which SavePackage completes
// downloads. SavePackage always uses its own Finish() to mark downloads
// complete.
void DownloadUrlJob::MaybeCompleteDownload() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(!is_save_package_download_);

  if (!IsDownloadReadyForCompletion(
          base::Bind(&DownloadUrlJob::MaybeCompleteDownload,
                     weak_ptr_factory_.GetWeakPtr())))
    return;
  // Confirm we're in the proper set of states to be here; have all data, have a
  // history handle, (validated or safe).
  DCHECK_EQ(IN_PROGRESS_INTERNAL, state_);
  DCHECK(!IsDangerous());
  DCHECK(all_data_saved_);

  OnDownloadCompleting();
}

// Called by MaybeCompleteDownload() when it has determined that the download
// is ready for completion.
void DownloadUrlJob::OnDownloadCompleting() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  if (state_ != IN_PROGRESS_INTERNAL)
    return;

  DVLOG(20) << __func__ << "() " << DebugString(true);
  DCHECK(!GetTargetFilePath().empty());
  DCHECK(!IsDangerous());

  // TODO(rdsmith/benjhayden): Remove as part of SavePackage integration.
  if (is_save_package_download_) {
    // Avoid doing anything on the file thread; there's nothing we control
    // there.  Strictly speaking, this skips giving the embedder a chance to
    // open the download.  But on a save package download, there's no real
    // concept of opening.
    Completed();
    return;
  }

  DCHECK(download_file_.get());
  // Unilaterally rename; even if it already has the right name,
  // we need theannotation.
  DownloadFile::RenameCompletionCallback callback =
      base::Bind(&DownloadUrlJob::OnDownloadRenamedToFinalName,
                 weak_ptr_factory_.GetWeakPtr());
  BrowserThread::PostTask(
      BrowserThread::FILE, FROM_HERE,
      base::Bind(&DownloadFile::RenameAndAnnotate,
                 base::Unretained(download_file_.get()), GetTargetFilePath(),
                 delegate_->GetApplicationClientIdForFileScanning(), GetURL(),
                 GetReferrerUrl(), callback));
}

void DownloadUrlJob::OnDownloadRenamedToFinalName(
    DownloadInterruptReason reason,
    const base::FilePath& full_path) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK(!is_save_package_download_);

  // If a cancel or interrupt hit, we'll cancel the DownloadFile, which
  // will result in deleting the file on the file thread.  So we don't
  // care about the name having been changed.
  if (state_ != IN_PROGRESS_INTERNAL)
    return;

  DVLOG(20) << __func__ << "() full_path = \"" << full_path.value() << "\" "
            << DebugString(false);

  if (DOWNLOAD_INTERRUPT_REASON_NONE != reason) {
    // Failure to perform the final rename is considered fatal. TODO(asanka): It
    // may not be, in which case we should figure out whether we can recover the
    // state.
    InterruptAndDiscardPartialState(reason);
    UpdateObservers();
    return;
  }

  DCHECK(target_path_ == full_path);

  if (full_path != current_path_) {
    // full_path is now the current and target file path.
    DCHECK(!full_path.empty());
    SetFullPath(full_path);
  }

  // Complete the download and release the DownloadFile.
  DCHECK(download_file_);
  ReleaseDownloadFile(false);

  // We're not completely done with the download item yet, but at this
  // point we're committed to complete the download.  Cancels (or Interrupts,
  // though it's not clear how they could happen) after this point will be
  // ignored.
  TransitionTo(COMPLETING_INTERNAL);

  if (delegate_->ShouldOpenDownload(
          this, base::Bind(&DownloadUrlJob::DelayedDownloadOpened,
                           weak_ptr_factory_.GetWeakPtr()))) {
    Completed();
  } else {
    delegate_delayed_complete_ = true;
    UpdateObservers();
  }
}

void DownloadUrlJob::DelayedDownloadOpened(bool auto_opened) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  auto_opened_ = auto_opened;
  Completed();
}

void DownloadUrlJob::Completed() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  DVLOG(20) << __func__ << "() " << DebugString(false);

  DCHECK(all_data_saved_);
  end_time_ = base::Time::Now();
  TransitionTo(COMPLETE_INTERNAL);
  RecordDownloadCompleted(start_tick_, received_bytes_);

  if (auto_opened_) {
    // If it was already handled by the delegate, do nothing.
  } else if (GetOpenWhenComplete() || ShouldOpenFileBasedOnExtension() ||
             IsTemporary()) {
    // If the download is temporary, like in drag-and-drop, do not open it but
    // we still need to set it auto-opened so that it can be removed from the
    // download shelf.
    if (!IsTemporary())
      OpenDownload();

    auto_opened_ = true;
  }
  UpdateObservers();
}

// **** End of Download progression cascade

void DownloadUrlJob::InterruptAndDiscardPartialState(
    DownloadInterruptReason reason) {
  InterruptWithPartialState(0, std::unique_ptr<crypto::SecureHash>(), reason);
}

void DownloadUrlJob::InterruptWithPartialState(
    int64_t bytes_so_far,
    std::unique_ptr<crypto::SecureHash> hash_state,
    DownloadInterruptReason reason) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DCHECK_NE(DOWNLOAD_INTERRUPT_REASON_NONE, reason);
  DVLOG(20) << __func__
            << "() reason:" << DownloadInterruptReasonToString(reason)
            << " bytes_so_far:" << bytes_so_far
            << " hash_state:" << (hash_state ? "Valid" : "Invalid")
            << " this=" << DebugString(true);

  // Somewhat counter-intuitively, it is possible for us to receive an
  // interrupt after we've already been interrupted.  The generation of
  // interrupts from the file thread Renames and the generation of
  // interrupts from disk writes go through two different mechanisms (driven
  // by rename requests from UI thread and by write requests from IO thread,
  // respectively), and since we choose not to keep state on the File thread,
  // this is the place where the races collide.  It's also possible for
  // interrupts to race with cancels.
  switch (state_) {
    case CANCELLED_INTERNAL:
    // If the download is already cancelled, then there's no point in
    // transitioning out to interrupted.
    case COMPLETING_INTERNAL:
    case COMPLETE_INTERNAL:
      // Already complete.
      return;

    case INITIAL_INTERNAL:
    case MAX_DOWNLOAD_INTERNAL_STATE:
      NOTREACHED();
      return;

    case INTERRUPTED_TARGET_PENDING_INTERNAL:
    case IN_PROGRESS_INTERNAL:
    case TARGET_PENDING_INTERNAL:
    case TARGET_RESOLVED_INTERNAL:
      // last_reason_ needs to be set for GetResumeMode() to work.
      last_reason_ = reason;

      if (download_file_) {
        ResumeMode resume_mode = GetResumeMode();
        ReleaseDownloadFile(resume_mode != RESUME_MODE_IMMEDIATE_CONTINUE &&
                            resume_mode != RESUME_MODE_USER_CONTINUE);
      }
      break;

    case RESUMING_INTERNAL:
    case INTERRUPTED_INTERNAL:
      DCHECK(!download_file_);
      // The first non-cancel interrupt reason wins in cases where multiple
      // things go wrong.
      if (reason != DOWNLOAD_INTERRUPT_REASON_USER_CANCELED &&
          reason != DOWNLOAD_INTERRUPT_REASON_USER_SHUTDOWN)
        return;

      last_reason_ = reason;
      if (!current_path_.empty()) {
        // There is no download file and this is transitioning from INTERRUPTED
        // to CANCELLED. The intermediate file is no longer usable, and should
        // be deleted.
        BrowserThread::PostTask(
            BrowserThread::FILE, FROM_HERE,
            base::Bind(base::IgnoreResult(&DeleteDownloadedFile),
                       current_path_));
        current_path_.clear();
      }
      break;
  }

  // Reset all data saved, as even if we did save all the data we're going to go
  // through another round of downloading when we resume. There's a potential
  // problem here in the abstract, as if we did download all the data and then
  // run into a continuable error, on resumption we won't download any more
  // data.  However, a) there are currently no continuable errors that can occur
  // after we download all the data, and b) if there were, that would probably
  // simply result in a null range request, which would generate a
  // DestinationCompleted() notification from the DownloadFile, which would
  // behave properly with setting all_data_saved_ to false here.
  all_data_saved_ = false;

  if (current_path_.empty()) {
    hash_state_.reset();
    hash_.clear();
    received_bytes_ = 0;
  } else {
    UpdateProgress(bytes_so_far, 0);
    SetHashState(std::move(hash_state));
  }

  if (request_handle_)
    request_handle_->CancelRequest();

  if (reason == DOWNLOAD_INTERRUPT_REASON_USER_CANCELED ||
      reason == DOWNLOAD_INTERRUPT_REASON_USER_SHUTDOWN) {
    if (IsDangerous()) {
      RecordDangerousDownloadDiscard(
          reason == DOWNLOAD_INTERRUPT_REASON_USER_CANCELED
              ? DOWNLOAD_DISCARD_DUE_TO_USER_ACTION
              : DOWNLOAD_DISCARD_DUE_TO_SHUTDOWN,
          GetDangerType(), GetTargetFilePath());
    }

    RecordDownloadCount(CANCELLED_COUNT);
    TransitionTo(CANCELLED_INTERNAL);
    return;
  }

  RecordDownloadInterrupted(reason, received_bytes_, total_bytes_);
  if (!GetWebContents())
    RecordDownloadCount(INTERRUPTED_WITHOUT_WEBCONTENTS);

  TransitionTo(INTERRUPTED_INTERNAL);
  AutoResumeIfValid();
}

void DownloadUrlJob::UpdateProgress(int64_t bytes_so_far,
                                    int64_t bytes_per_sec) {
  received_bytes_ = bytes_so_far;
  bytes_per_sec_ = bytes_per_sec;

  // If we've received more data than we were expecting (bad server info?),
  // revert to 'unknown size mode'.
  if (received_bytes_ > total_bytes_)
    total_bytes_ = 0;
}

void DownloadUrlJob::SetHashState(
    std::unique_ptr<crypto::SecureHash> hash_state) {
  hash_state_ = std::move(hash_state);
  if (!hash_state_) {
    hash_.clear();
    return;
  }

  std::unique_ptr<crypto::SecureHash> clone_of_hash_state(hash_state_->Clone());
  std::vector<char> hash_value(clone_of_hash_state->GetHashLength());
  clone_of_hash_state->Finish(&hash_value.front(), hash_value.size());
  hash_.assign(hash_value.begin(), hash_value.end());
}

void DownloadUrlJob::ReleaseDownloadFile(bool destroy_file) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DVLOG(20) << __func__ << "() destroy_file:" << destroy_file;

  if (destroy_file) {
    BrowserThread::PostTask(
        BrowserThread::FILE, FROM_HERE,
        // Will be deleted at end of task execution.
        base::Bind(&DownloadFileCancel, base::Passed(&download_file_)));
    // Avoid attempting to reuse the intermediate file by clearing out
    // current_path_.
    current_path_.clear();
  } else {
    BrowserThread::PostTask(
        BrowserThread::FILE, FROM_HERE,
        base::Bind(base::IgnoreResult(&DownloadFileDetach),
                   // Will be deleted at end of task execution.
                   base::Passed(&download_file_)));
  }
  // Don't accept any more messages from the DownloadFile, and null
  // out any previous "all data received".  This also breaks links to
  // other entities we've given out weak pointers to.
  weak_ptr_factory_.InvalidateWeakPtrs();
}

bool DownloadUrlJob::IsDownloadReadyForCompletion(
    const base::Closure& state_change_notification) {
  // If the download hasn't progressed to the IN_PROGRESS state, then it's not
  // ready for completion.
  if (state_ != IN_PROGRESS_INTERNAL)
    return false;

  // If we don't have all the data, the download is not ready for
  // completion.
  if (!AllDataSaved())
    return false;

  // If the download is dangerous, but not yet validated, it's not ready for
  // completion.
  if (IsDangerous())
    return false;

  // Check for consistency before invoking delegate. Since there are no pending
  // target determination calls and the download is in progress, both the target
  // and current paths should be non-empty and they should point to the same
  // directory.
  DCHECK(!target_path_.empty());
  DCHECK(!current_path_.empty());
  DCHECK(target_path_.DirName() == current_path_.DirName());

  // Give the delegate a chance to hold up a stop sign.  It'll call
  // use back through the passed callback if it does and that state changes.
  if (!delegate_->ShouldCompleteDownload(this, state_change_notification))
    return false;

  return true;
}

void DownloadUrlJob::TransitionTo(DownloadInternalState new_state) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  if (state_ == new_state)
    return;

  DownloadInternalState old_state = state_;
  state_ = new_state;

  DCHECK(is_save_package_download_
             ? IsValidSavePackageStateTransition(old_state, new_state)
             : IsValidStateTransition(old_state, new_state))
      << "Invalid state transition from:" << DebugDownloadStateString(old_state)
      << " to:" << DebugDownloadStateString(new_state);

  switch (state_) {
    case INITIAL_INTERNAL:
      NOTREACHED();
      break;

    case TARGET_PENDING_INTERNAL:
    case TARGET_RESOLVED_INTERNAL:
    case INTERRUPTED_TARGET_PENDING_INTERNAL:
      break;

    case IN_PROGRESS_INTERNAL:
      DCHECK(!current_path_.empty()) << "Current output path must be known.";
      DCHECK(!target_path_.empty()) << "Target path must be known.";
      DCHECK(current_path_.DirName() == target_path_.DirName())
          << "Current output directory must match target directory.";
      DCHECK(download_file_) << "Output file must be owned by download item.";
      DCHECK(request_handle_) << "Download source must be active.";
      DCHECK(!is_paused_) << "At the time a download enters IN_PROGRESS state, "
                             "it must not be paused.";
      break;

    case COMPLETING_INTERNAL:
      DCHECK(all_data_saved_) << "All data must be saved prior to completion.";
      DCHECK(!download_file_)
          << "Download file must be released prior to completion.";
      DCHECK(!target_path_.empty()) << "Target path must be known.";
      DCHECK(current_path_ == target_path_)
          << "Current output path must match target path.";

      net_log_.AddEvent(
          net::NetLogEventType::DOWNLOAD_ITEM_COMPLETING,
          base::Bind(&ItemCompletingNetLogCallback, received_bytes_, &hash_));
      break;

    case COMPLETE_INTERNAL:
      net_log_.AddEvent(
          net::NetLogEventType::DOWNLOAD_ITEM_FINISHED,
          base::Bind(&ItemFinishedNetLogCallback, auto_opened_));
      break;

    case INTERRUPTED_INTERNAL:
      net_log_.AddEvent(net::NetLogEventType::DOWNLOAD_ITEM_INTERRUPTED,
                              base::Bind(&ItemInterruptedNetLogCallback,
                                         last_reason_, received_bytes_));
      break;

    case RESUMING_INTERNAL:
      net_log_.AddEvent(net::NetLogEventType::DOWNLOAD_ITEM_RESUMED,
                              base::Bind(&ItemResumingNetLogCallback, false,
                                         last_reason_, received_bytes_));
      break;

    case CANCELLED_INTERNAL:
      net_log_.AddEvent(
          net::NetLogEventType::DOWNLOAD_ITEM_CANCELED,
          base::Bind(&ItemCanceledNetLogCallback, received_bytes_));
      break;

    case MAX_DOWNLOAD_INTERNAL_STATE:
      NOTREACHED();
      break;
  }

  DVLOG(20) << __func__ << "() from:" << DebugDownloadStateString(old_state)
            << " to:" << DebugDownloadStateString(state_)
            << " this = " << DebugString(true);
  bool is_done =
      (state_ == COMPLETE_INTERNAL || state_ == INTERRUPTED_INTERNAL ||
       state_ == RESUMING_INTERNAL || state_ == CANCELLED_INTERNAL);
  bool was_done =
      (old_state == COMPLETE_INTERNAL || old_state == INTERRUPTED_INTERNAL ||
       old_state == RESUMING_INTERNAL || old_state == CANCELLED_INTERNAL);

  // Termination
  if (is_done && !was_done)
    net_log_.EndEvent(net::NetLogEventType::DOWNLOAD_ITEM_ACTIVE);

  // Resumption
  if (was_done && !is_done) {
    std::string file_name(target_path_.BaseName().AsUTF8Unsafe());
    net_log_.BeginEvent(net::NetLogEventType::DOWNLOAD_ITEM_ACTIVE,
                              base::Bind(&ItemActivatedNetLogCallback, this,
                                         SRC_ACTIVE_DOWNLOAD, &file_name));
  }
}

void DownloadUrlJob::SetDangerType(DownloadDangerType danger_type) {
  if (danger_type != danger_type_) {
    net_log_.AddEvent(
        net::NetLogEventType::DOWNLOAD_ITEM_SAFETY_STATE_UPDATED,
        base::Bind(&ItemCheckedNetLogCallback, danger_type));
  }
  // Only record the Malicious UMA stat if it's going from {not malicious} ->
  // {malicious}.
  if ((danger_type_ == DOWNLOAD_DANGER_TYPE_NOT_DANGEROUS ||
       danger_type_ == DOWNLOAD_DANGER_TYPE_DANGEROUS_FILE ||
       danger_type_ == DOWNLOAD_DANGER_TYPE_UNCOMMON_CONTENT ||
       danger_type_ == DOWNLOAD_DANGER_TYPE_MAYBE_DANGEROUS_CONTENT) &&
      (danger_type == DOWNLOAD_DANGER_TYPE_DANGEROUS_HOST ||
       danger_type == DOWNLOAD_DANGER_TYPE_DANGEROUS_URL ||
       danger_type == DOWNLOAD_DANGER_TYPE_DANGEROUS_CONTENT ||
       danger_type == DOWNLOAD_DANGER_TYPE_POTENTIALLY_UNWANTED)) {
    RecordMaliciousDownloadClassified(danger_type);
  }
  danger_type_ = danger_type;
}

void DownloadUrlJob::SetFullPath(const base::FilePath& new_path) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  DVLOG(20) << __func__ << "() new_path = \"" << new_path.value() << "\" "
            << DebugString(true);
  DCHECK(!new_path.empty());

  net_log_.AddEvent(
      net::NetLogEventType::DOWNLOAD_ITEM_RENAMED,
      base::Bind(&ItemRenamedNetLogCallback, &current_path_, &new_path));

  current_path_ = new_path;
}

void DownloadUrlJob::AutoResumeIfValid() {
  DVLOG(20) << __func__ << "() " << DebugString(true);
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  ResumeMode mode = GetResumeMode();

  if (mode != RESUME_MODE_IMMEDIATE_RESTART &&
      mode != RESUME_MODE_IMMEDIATE_CONTINUE) {
    return;
  }

  auto_resume_count_++;

  ResumeInterruptedDownload(ResumptionRequestSource::AUTOMATIC);
}

void DownloadUrlJob::ResumeInterruptedDownload(ResumptionRequestSource source) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  // If we're not interrupted, ignore the request; our caller is drunk.
  if (state_ != INTERRUPTED_INTERNAL)
    return;

  // We are starting a new request. Shake off all pending operations.
  DCHECK(!download_file_);
  weak_ptr_factory_.InvalidateWeakPtrs();

  // Reset the appropriate state if restarting.
  ResumeMode mode = GetResumeMode();
  if (mode == RESUME_MODE_IMMEDIATE_RESTART ||
      mode == RESUME_MODE_USER_RESTART) {
    received_bytes_ = 0;
    last_modified_time_.clear();
    etag_.clear();
    hash_.clear();
    hash_state_.reset();
  }

  StoragePartition* storage_partition =
      BrowserContext::GetStoragePartitionForSite(GetBrowserContext(),
                                                 site_url_);

  // Avoid using the WebContents even if it's still around. Resumption requests
  // are consistently routed through the no-renderer code paths so that the
  // request will not be dropped if the WebContents (and by extension, the
  // associated renderer) goes away before a response is received.
  std::unique_ptr<DownloadUrlParameters> download_params(
      new DownloadUrlParameters(GetURL(),
                                storage_partition->GetURLRequestContext()));
  download_params->set_file_path(GetFullPath());
  download_params->set_offset(GetReceivedBytes());
  download_params->set_last_modified(GetLastModifiedTime());
  download_params->set_etag(GetETag());
  download_params->set_hash_of_partial_file(hash_);
  download_params->set_hash_state(std::move(hash_state_));

  // Note that resumed downloads disallow redirects. Hence the referrer URL
  // (which is the contents of the Referer header for the last download request)
  // will only be sent to the URL returned by GetURL().
  download_params->set_referrer(
      Referrer(GetReferrerUrl(), blink::WebReferrerPolicyAlways));

  TransitionTo(RESUMING_INTERNAL);
  RecordDownloadSource(source == ResumptionRequestSource::USER
                           ? INITIATED_BY_MANUAL_RESUMPTION
                           : INITIATED_BY_AUTOMATIC_RESUMPTION);
  delegate_->ResumeInterruptedDownload(std::move(download_params), GetId());
  // Just in case we were interrupted while paused.
  is_paused_ = false;
}

// static
DownloadItem::DownloadState DownloadUrlJob::InternalToExternalState(
    DownloadInternalState internal_state) {
  switch (internal_state) {
    case INITIAL_INTERNAL:
    case TARGET_PENDING_INTERNAL:
    case TARGET_RESOLVED_INTERNAL:
    case INTERRUPTED_TARGET_PENDING_INTERNAL:
    // TODO(asanka): Introduce an externally visible state to distinguish
    // between the above states and IN_PROGRESS_INTERNAL. The latter (the
    // state where the download is active and has a known target) is the state
    // that most external users are interested in.
    case IN_PROGRESS_INTERNAL:
      return IN_PROGRESS;
    case COMPLETING_INTERNAL:
      return IN_PROGRESS;
    case COMPLETE_INTERNAL:
      return COMPLETE;
    case CANCELLED_INTERNAL:
      return CANCELLED;
    case INTERRUPTED_INTERNAL:
      return INTERRUPTED;
    case RESUMING_INTERNAL:
      return IN_PROGRESS;
    case MAX_DOWNLOAD_INTERNAL_STATE:
      break;
  }
  NOTREACHED();
  return MAX_DOWNLOAD_STATE;
}

// static
DownloadUrlJob::DownloadInternalState DownloadUrlJob::ExternalToInternalState(
    DownloadState external_state) {
  switch (external_state) {
    case IN_PROGRESS:
      return IN_PROGRESS_INTERNAL;
    case COMPLETE:
      return COMPLETE_INTERNAL;
    case CANCELLED:
      return CANCELLED_INTERNAL;
    case INTERRUPTED:
      return INTERRUPTED_INTERNAL;
    default:
      NOTREACHED();
  }
  return MAX_DOWNLOAD_INTERNAL_STATE;
}

// static
bool DownloadUrlJob::IsValidSavePackageStateTransition(
    DownloadInternalState from,
    DownloadInternalState to) {
#if DCHECK_IS_ON()
  switch (from) {
    case INITIAL_INTERNAL:
    case TARGET_PENDING_INTERNAL:
    case INTERRUPTED_TARGET_PENDING_INTERNAL:
    case TARGET_RESOLVED_INTERNAL:
    case COMPLETING_INTERNAL:
    case COMPLETE_INTERNAL:
    case INTERRUPTED_INTERNAL:
    case RESUMING_INTERNAL:
    case CANCELLED_INTERNAL:
      return false;

    case IN_PROGRESS_INTERNAL:
      return to == CANCELLED_INTERNAL || to == COMPLETE_INTERNAL;

    case MAX_DOWNLOAD_INTERNAL_STATE:
      NOTREACHED();
  }
  return false;
#else
  return true;
#endif
}

// static
bool DownloadUrlJob::IsValidStateTransition(DownloadInternalState from,
                                            DownloadInternalState to) {
#if DCHECK_IS_ON()
  switch (from) {
    case INITIAL_INTERNAL:
      return to == TARGET_PENDING_INTERNAL ||
             to == INTERRUPTED_TARGET_PENDING_INTERNAL;

    case TARGET_PENDING_INTERNAL:
      return to == INTERRUPTED_TARGET_PENDING_INTERNAL ||
             to == TARGET_RESOLVED_INTERNAL || to == CANCELLED_INTERNAL;

    case INTERRUPTED_TARGET_PENDING_INTERNAL:
      return to == INTERRUPTED_INTERNAL || to == CANCELLED_INTERNAL;

    case TARGET_RESOLVED_INTERNAL:
      return to == IN_PROGRESS_INTERNAL || to == INTERRUPTED_INTERNAL ||
             to == CANCELLED_INTERNAL;

    case IN_PROGRESS_INTERNAL:
      return to == COMPLETING_INTERNAL || to == CANCELLED_INTERNAL ||
             to == INTERRUPTED_INTERNAL;

    case COMPLETING_INTERNAL:
      return to == COMPLETE_INTERNAL;

    case COMPLETE_INTERNAL:
      return false;

    case INTERRUPTED_INTERNAL:
      return to == RESUMING_INTERNAL || to == CANCELLED_INTERNAL;

    case RESUMING_INTERNAL:
      return to == TARGET_PENDING_INTERNAL ||
             to == INTERRUPTED_TARGET_PENDING_INTERNAL ||
             to == TARGET_RESOLVED_INTERNAL || to == CANCELLED_INTERNAL;

    case CANCELLED_INTERNAL:
      return false;

    case MAX_DOWNLOAD_INTERNAL_STATE:
      NOTREACHED();
  }
  return false;
#else
  return true;
#endif  // DCHECK_IS_ON()
}

const char* DownloadUrlJob::DebugDownloadStateString(
    DownloadInternalState state) {
  switch (state) {
    case INITIAL_INTERNAL:
      return "INITIAL";
    case TARGET_PENDING_INTERNAL:
      return "TARGET_PENDING";
    case INTERRUPTED_TARGET_PENDING_INTERNAL:
      return "INTERRUPTED_TARGET_PENDING";
    case TARGET_RESOLVED_INTERNAL:
      return "TARGET_RESOLVED";
    case IN_PROGRESS_INTERNAL:
      return "IN_PROGRESS";
    case COMPLETING_INTERNAL:
      return "COMPLETING";
    case COMPLETE_INTERNAL:
      return "COMPLETE";
    case CANCELLED_INTERNAL:
      return "CANCELLED";
    case INTERRUPTED_INTERNAL:
      return "INTERRUPTED";
    case RESUMING_INTERNAL:
      return "RESUMING";
    case MAX_DOWNLOAD_INTERNAL_STATE:
      break;
  };
  NOTREACHED() << "Unknown download state " << state;
  return "unknown";
}

const char* DownloadUrlJob::DebugResumeModeString(ResumeMode mode) {
  switch (mode) {
    case RESUME_MODE_INVALID:
      return "INVALID";
    case RESUME_MODE_IMMEDIATE_CONTINUE:
      return "IMMEDIATE_CONTINUE";
    case RESUME_MODE_IMMEDIATE_RESTART:
      return "IMMEDIATE_RESTART";
    case RESUME_MODE_USER_CONTINUE:
      return "USER_CONTINUE";
    case RESUME_MODE_USER_RESTART:
      return "USER_RESTART";
  }
  NOTREACHED() << "Unknown resume mode " << mode;
  return "unknown";
}

}  // namespace content
