// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_DOWNLOAD_DOWNLOAD_URL_JOB_H_
#define CONTENT_BROWSER_DOWNLOAD_DOWNLOAD_URL_JOB_H_

#include <stdint.h>

#include <memory>
#include <string>

#include "base/callback_forward.h"
#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/time/time.h"
#include "content/browser/download/download_destination_observer.h"
#include "content/browser/download/download_net_log_parameters.h"
#include "content/browser/download/download_request_handle.h"
#include "content/common/content_export.h"
#include "content/public/browser/download_interrupt_reasons.h"
#include "content/public/browser/download_item.h"
#include "content/public/browser/download_job.h"
#include "net/log/net_log_with_source.h"
#include "url/gurl.h"

namespace content {
class DownloadFile;
class DownloadUrlJobDelegate;

// See download_item.h for usage.
class CONTENT_EXPORT DownloadUrlJob : public DownloadJob,
                                      public DownloadDestinationObserver {
 public:
  enum ResumeMode {
    RESUME_MODE_INVALID = 0,
    RESUME_MODE_IMMEDIATE_CONTINUE,
    RESUME_MODE_IMMEDIATE_RESTART,
    RESUME_MODE_USER_CONTINUE,
    RESUME_MODE_USER_RESTART
  };

  // The maximum number of attempts we will make to resume automatically.
  static const int kMaxAutoResumeAttempts;

  // Note that it is the responsibility of the caller to ensure that a
  // DownloadUrlJobDelegate passed to a DownloadUrlJob
  // constructor outlives the DownloadUrlJob.

  // Constructing for a regular download.  |bound_net_log| is constructed
  // externally for our use.
  DownloadUrlJob(DownloadUrlJobDelegate* delegate,
                 const DownloadSaveInfo& save_info,
                 std::unique_ptr<DownloadItem::ResponseInfo> response_info,
                 std::unique_ptr<DownloadRequestHandleInterface> request_handle,
                 const net::NetLogWithSource& request_bound_net_log,
                 const net::NetLogWithSource& download_bound_net_log);

  ~DownloadUrlJob() override;

  // All remaining public interfaces virtual to allow for DownloadUrlJob
  // mocks.

  // Determines the resume mode for an interrupted download. Requires
  // last_reason_ to be set, but doesn't require the download to be in
  // INTERRUPTED state.
  ResumeMode GetResumeMode() const;

  // State transition operations on regular downloads --------------------------

  // Start the download.
  // |download_file| is the associated file on the storage medium.
  // |req_handle| is the new request handle associated with the download.
  // |new_create_info| is a DownloadCreateInfo containing the new response
  // parameters. It may be different from the DownloadCreateInfo used to create
  // the DownloadItem if Start() is being called in response for a download
  // resumption request.
  void SetDownloadFile(std::unique_ptr<DownloadFile> download_file);

  // TODO(asanka): Move interrupted download handling to a separate job.
  void SetInitialIterruptedState(
      DownloadInterruptReason interrupt_reason,
      std::unique_ptr<DownloadSaveInfo> interrupted_save_info);

  // Needed because of intertwining with DownloadManagerImpl -------------------

  // TODO(rdsmith): Unwind DownloadManagerImpl and DownloadUrlJob,
  // removing these from the public interface.

  // Notify observers that this item is being removed by the user.
  void NotifyRemoved();

  void OnDownloadedFileRemoved();

  // Provide a weak pointer reference to a DownloadDestinationObserver
  // for use by download destinations.
  base::WeakPtr<DownloadDestinationObserver> DestinationObserverAsWeakPtr();

  // Get the download's NetLogWithSource.
  const net::NetLogWithSource& GetNetLogWithSource() const;

  // DownloadJob
  void OnAttached() override;
  void OnBeforeDetach() override;
  void Cancel(bool user_cancel) override;
  void Pause() override;
  void Resume() override;
  bool CanOpen() const override;
  bool CanResume() const override;
  bool CanShowInFolder() const override;
  bool IsActive() const override;
  bool IsPaused() const override;
  int PercentComplete() const override;
  int64_t CurrentSpeed() const override;
  bool TimeRemaining(base::TimeDelta* remaining) const override;
  WebContents* GetWebContents() const override;
  std::string DebugString(bool verbose) const override;
  bool IsSavePackageDownload() const override;

 private:
  // DownloadDestinationObserver
  void DestinationUpdate(int64_t bytes_so_far, int64_t bytes_per_sec) override;
  void DestinationError(
      DownloadInterruptReason reason,
      int64_t bytes_so_far,
      std::unique_ptr<crypto::SecureHash> hash_state) override;
  void DestinationCompleted(
      int64_t total_bytes,
      std::unique_ptr<crypto::SecureHash> hash_state) override;

  // Fine grained states of a download.
  //
  // New downloads can be created in the following states:
  //
  //     INITIAL_INTERNAL:        All active new downloads.
  //
  //     COMPLETE_INTERNAL:       Downloads restored from persisted state.
  //     CANCELLED_INTERNAL:      - do -
  //     INTERRUPTED_INTERNAL:    - do -
  //
  //     IN_PROGRESS_INTERNAL:    SavePackage downloads.
  //
  // On debug builds, state transitions can be verified via
  // IsValidStateTransition() and IsValidSavePackageStateTransition(). Allowed
  // state transitions are described below, both for normal downloads and
  // SavePackage downloads.
  enum DownloadInternalState {
    // Initial state. Regular downloads are created in this state until the
    // Start() call is received.
    //
    // Transitions to (regular):
    //   TARGET_PENDING_INTERNAL: After a successful Start() call.
    //   INTERRUPTED_TARGET_PENDING_INTERNAL: After a failed Start() call.
    //
    // Transitions to (SavePackage):
    //   <n/a>                    SavePackage downloads never reach this state.
    INITIAL_INTERNAL,

    // Embedder is in the process of determining the target of the download.
    // Since the embedder is sensitive to state transitions during this time,
    // any DestinationError/DestinationCompleted events are deferred until
    // TARGET_RESOLVED_INTERNAL.
    //
    // Transitions to (regular):
    //   TARGET_RESOLVED_INTERNAL: Once the embedder invokes the callback.
    //   INTERRUPTED_TARGET_PENDING_INTERNAL: An error occurred prior to target
    //                            determination.
    //   CANCELLED_INTERNAL:      Cancelled.
    //
    // Transitions to (SavePackage):
    //   <n/a>                    SavePackage downloads never reach this state.
    TARGET_PENDING_INTERNAL,

    // Embedder is in the process of determining the target of the download, and
    // the download is in an interrupted state. The interrupted state is not
    // exposed to the emedder until target determination is complete.
    //
    // Transitions to (regular):
    //   INTERRUPTED_INTERNAL:    Once the target is determined, the download
    //                            is marked as interrupted.
    //   CANCELLED_INTERNAL:      Cancelled.
    //
    // Transitions to (SavePackage):
    //   <n/a>                    SavePackage downloads never reach this state.
    INTERRUPTED_TARGET_PENDING_INTERNAL,

    // Embedder has completed target determination. It is now safe to resolve
    // the download target as well as process deferred DestinationError events.
    // This state is differs from TARGET_PENDING_INTERNAL due to it being
    // allowed to transition to INTERRUPTED_INTERNAL, and it's different from
    // IN_PROGRESS_INTERNAL in that entering this state doesn't require having
    // a valid target. This state is transient (i.e. DownloadUrlJob will
    // transition out of it before yielding execution). It's only purpose in
    // life is to ensure the integrity of state transitions.
    //
    // Transitions to (regular):
    //   IN_PROGRESS_INTERNAL:    Target successfully determined. The incoming
    //                            data stream can now be written to the target.
    //   INTERRUPTED_INTERNAL:    Either the target determination or one of the
    //                            deferred signals indicated that the download
    //                            should be interrupted.
    //   CANCELLED_INTERNAL:      User cancelled the download or there was a
    //                            deferred Cancel() call.
    //
    // Transitions to (SavePackage):
    //   <n/a>                    SavePackage downloads never reach this state.
    TARGET_RESOLVED_INTERNAL,

    // Download target is known and the data can be transferred from our source
    // to our sink.
    //
    // Transitions to (regular):
    //   COMPLETING_INTERNAL:     On final rename completion.
    //   CANCELLED_INTERNAL:      On cancel.
    //   INTERRUPTED_INTERNAL:    On interrupt.
    //
    // Transitions to (SavePackage):
    //   COMPLETE_INTERNAL:       On completion.
    //   CANCELLED_INTERNAL:      On cancel.
    IN_PROGRESS_INTERNAL,

    // Between commit point (dispatch of download file release) and completed.
    // Embedder may be opening the file in this state.
    //
    // Transitions to (regular):
    //   COMPLETE_INTERNAL:       On successful completion.
    //
    // Transitions to (SavePackage):
    //   <n/a>                    SavePackage downloads never reach this state.
    COMPLETING_INTERNAL,

    // After embedder has had a chance to auto-open.  User may now open
    // or auto-open based on extension.
    //
    // Transitions to (regular):
    //   <none>                   Terminal state.
    //
    // Transitions to (SavePackage):
    //   <none>                   Terminal state.
    COMPLETE_INTERNAL,

    // An error has interrupted the download.
    //
    // Transitions to (regular):
    //   RESUMING_INTERNAL:       On resumption.
    //   CANCELLED_INTERNAL:      On cancel.
    //
    // Transitions to (SavePackage):
    //   <n/a>                    SavePackage downloads never reach this state.
    INTERRUPTED_INTERNAL,

    // A request to resume this interrupted download is in progress.
    //
    // Transitions to (regular):
    //   TARGET_PENDING_INTERNAL: Once a server response is received from a
    //                            resumption.
    //   INTERRUPTED_TARGET_PENDING_INTERNAL: A server response was received,
    //                            but it indicated an error, and the download
    //                            needs to go through target determination.
    //   TARGET_RESOLVED_INTERNAL: A resumption attempt received an error
    //                            but it was not necessary to go through target
    //                            determination.
    //   CANCELLED_INTERNAL:      On cancel.
    //
    // Transitions to (SavePackage):
    //   <n/a>                    SavePackage downloads never reach this state.
    RESUMING_INTERNAL,

    // User has cancelled the download.
    //
    // Transitions to (regular):
    //   <none>                   Terminal state.
    //
    // Transitions to (SavePackage):
    //   <none>                   Terminal state.
    CANCELLED_INTERNAL,

    MAX_DOWNLOAD_INTERNAL_STATE,
  };

  // Normal progression of a download ------------------------------------------

  // These are listed in approximately chronological order.  There are also
  // public methods involved in normal download progression; see
  // the implementation ordering in download_item_impl.cc.

  // Construction common to all constructors. |active| should be true for new
  // downloads and false for downloads from the history.
  // |download_type| indicates to the net log system what kind of download
  // this is.
  void Init(bool active);

  // Callback from file thread when we initialize the DownloadFile.
  void OnDownloadFileInitialized(DownloadInterruptReason result);

  // Called to determine the target path. Will cause OnDownloadTargetDetermined
  // to be called when the target information is available.
  void DetermineDownloadTarget();

  // Called when the target path has been determined. |target_path| is the
  // suggested target path. |disposition| indicates how the target path should
  // be used (see TargetDisposition). |danger_type| is the danger level of
  // |target_path| as determined by the caller. |intermediate_path| is the path
  // to use to store the download until OnDownloadCompleting() is called.
  void OnDownloadTargetDetermined(const base::FilePath& target_path,
                                  DownloadItem::TargetDisposition disposition,
                                  DownloadDangerType danger_type,
                                  const base::FilePath& intermediate_path);

  void OnDownloadRenamedToIntermediateName(DownloadInterruptReason reason,
                                           const base::FilePath& full_path);

  // If all pre-requisites have been met, complete download processing, i.e. do
  // internal cleanup, file rename, and potentially auto-open.  (Dangerous
  // downloads still may block on user acceptance after this point.)
  void MaybeCompleteDownload();

  // Called when the download is ready to complete.
  // This may perform final rename if necessary and will eventually call
  // DownloadItem::Completed().
  void OnDownloadCompleting();

  void OnDownloadRenamedToFinalName(DownloadInterruptReason reason,
                                    const base::FilePath& full_path);

  // Called if the embedder took over opening a download, to indicate that
  // the download has been opened.
  void DelayedDownloadOpened(bool auto_opened);

  // Called when the entire download operation (including renaming etc.)
  // is completed.
  void Completed();

  // Helper routines -----------------------------------------------------------

  // Indicate that an error has occurred on the download. Discards partial
  // state. The interrupted download will not be considered continuable, but may
  // be restarted.
  void InterruptAndDiscardPartialState(DownloadInterruptReason reason);

  // Indiates that an error has occurred on the download. The |bytes_so_far| and
  // |hash_state| should correspond to the state of the DownloadFile. If the
  // interrupt reason allows, this partial state may be allowed to continue the
  // interrupted download upon resumption.
  void InterruptWithPartialState(int64_t bytes_so_far,
                                 std::unique_ptr<crypto::SecureHash> hash_state,
                                 DownloadInterruptReason reason);

  void UpdateProgress(int64_t bytes_so_far, int64_t bytes_per_sec);

  // Set |hash_| and |hash_state_| based on |hash_state|.
  void SetHashState(std::unique_ptr<crypto::SecureHash> hash_state);

  // Destroy the DownloadFile object.  If |destroy_file| is true, the file is
  // destroyed with it.  Otherwise, DownloadFile::Detach() is called before
  // object destruction to prevent file destruction. Destroying the file also
  // resets |current_path_|.
  void ReleaseDownloadFile(bool destroy_file);

  // Check if a download is ready for completion.  The callback provided
  // may be called at some point in the future if an external entity
  // state has change s.t. this routine should be checked again.
  bool IsDownloadReadyForCompletion(const base::Closure& state_change_notify);

  // Call to transition state; all state transitions should go through this.
  // |notify_action| specifies whether or not to call UpdateObservers() after
  // the state transition.
  void TransitionTo(DownloadInternalState new_state);

  // Set the |danger_type_| and invoke observers if necessary.
  void SetDangerType(DownloadDangerType danger_type);

  void SetFullPath(const base::FilePath& new_path);

  void AutoResumeIfValid();

  enum class ResumptionRequestSource { AUTOMATIC, USER };
  void ResumeInterruptedDownload(ResumptionRequestSource source);

  // Update origin information based on the response to a download resumption
  // request. Should only be called if the resumption request was successful.
  virtual void UpdateValidatorsOnResumption(
      const DownloadCreateInfo& new_create_info);

  static DownloadItem::DownloadState InternalToExternalState(
      DownloadInternalState internal_state);
  static DownloadInternalState ExternalToInternalState(
      DownloadItem::DownloadState external_state);

  // Debugging routines --------------------------------------------------------
  static const char* DebugDownloadStateString(DownloadInternalState state);
  static const char* DebugResumeModeString(ResumeMode mode);
  static bool IsValidSavePackageStateTransition(DownloadInternalState from,
                                                DownloadInternalState to);
  static bool IsValidStateTransition(DownloadInternalState from,
                                     DownloadInternalState to);

  DownloadItem::TargetDisposition target_disposition_ =
      DownloadItem::TARGET_DISPOSITION_OVERWRITE;

  // The handle to the request information.  Used for operations outside the
  // download system.
  std::unique_ptr<DownloadRequestHandleInterface> request_handle_;

  // Initialization info.
  std::unique_ptr<DownloadItem::ResponseInfo> initial_response_info_;

  // Start time for recording statistics.
  base::TimeTicks start_tick_;

  // The current state of this download.
  DownloadInternalState state_ = INITIAL_INTERNAL;
  DownloadInterruptReason pending_interrupt_reason_ =
      DOWNLOAD_INTERRUPT_REASON_NONE;
  std::unique_ptr<DownloadSaveInfo> interrupted_save_info_;

  // Our delegate.
  DownloadUrlJobDelegate* const delegate_;

  // Did the delegate delay calling Complete on this download?
  bool delegate_delayed_complete_ = false;

  // The following fields describe the current state of the download file.

  // DownloadFile associated with this download.  Note that this
  // pointer may only be used or destroyed on the FILE thread.
  // This pointer will be non-null only while the DownloadItem is in
  // the IN_PROGRESS state.
  std::unique_ptr<DownloadFile> download_file_;

  // Current speed. Calculated by the DownloadFile.
  int64_t bytes_per_sec_ = 0;

  // The number of times this download has been resumed automatically. Will be
  // reset to 0 if a resumption is performed in response to a Resume() call.
  int auto_resume_count_ = 0;

  // In the event of an interruption, the DownloadDestinationObserver interface
  // exposes the partial hash state. This state can be held by the download item
  // in case it's needed for resumption.
  std::unique_ptr<crypto::SecureHash> hash_state_;

  // Net log to use for this download.
  const net::NetLogWithSource net_log_;

  base::WeakPtrFactory<DownloadUrlJob> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(DownloadUrlJob);
};

}  // namespace content

#endif  // CONTENT_BROWSER_DOWNLOAD_DOWNLOAD_URL_JOB_H_
