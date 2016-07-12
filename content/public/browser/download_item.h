// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Each download is represented by a DownloadItem, and all DownloadItems
// are owned by the DownloadManager which maintains a global list of all
// downloads. DownloadItems are created when a user initiates a download,
// and exist for the duration of the browser life time.
//
// Download observers:
//   DownloadItem::Observer:
//     - allows observers to receive notifications about one download from start
//       to completion
// Use AddObserver() / RemoveObserver() on the appropriate download object to
// receive state updates.

#ifndef CONTENT_PUBLIC_BROWSER_DOWNLOAD_ITEM_H_
#define CONTENT_PUBLIC_BROWSER_DOWNLOAD_ITEM_H_

#include <stdint.h>

#include <map>
#include <string>
#include <vector>

#include "base/callback_forward.h"
#include "base/files/file_path.h"
#include "base/observer_list.h"
#include "base/strings/string16.h"
#include "base/supports_user_data.h"
#include "content/public/browser/download_danger_type.h"
#include "content/public/browser/download_interrupt_reasons.h"
#include "ui/base/page_transition_types.h"
#include "url/gurl.h"

namespace base {
class Time;
class TimeDelta;
}

namespace content {

class BrowserContext;
class DownloadItemDelegate;
class DownloadJob;
class DownloadManager;
class WebContents;

// One DownloadItem per download. This is the model class that stores all the
// state for a download. Multiple views, such as a tab's download shelf and the
// Destination tab's download view, may refer to a given DownloadItem.
//
// This is intended to be used only on the UI thread.
class CONTENT_EXPORT DownloadItem : public base::SupportsUserData {
 public:
  // A Java counterpart will be generated for this enum.
  // GENERATED_JAVA_ENUM_PACKAGE: org.chromium.content_public.browser
  enum DownloadState {
    // No job attached to download.
    DETACHED = -1,

    // Download is actively progressing.
    IN_PROGRESS = 0,

    // Download is completely finished.
    COMPLETE,

    // Download has been cancelled.
    CANCELLED,

    // This state indicates that the download has been interrupted.
    INTERRUPTED,

    // Maximum value.
    MAX_DOWNLOAD_STATE
  };

  // How the final target path should be used.
  enum TargetDisposition {
    TARGET_DISPOSITION_OVERWRITE, // Overwrite if the target already exists.
    TARGET_DISPOSITION_PROMPT     // Prompt the user for the actual
                                  // target. Implies
                                  // TARGET_DISPOSITION_OVERWRITE.
  };

  struct RequestInfo {
    // The chain of redirects that leading up to and including the final URL.
    std::vector<GURL> url_chain;

    // The URL of the page that initiated the download.
    GURL referrer_url;

    // Site URL for the site instance that initiated this download.
    GURL site_url;

    // The URL of the tab that initiated the download.
    GURL tab_url;

    // The URL of the referrer of the tab that initiated the download.
    GURL tab_referrer_url;

    // Filename suggestion from DownloadSaveInfo. It could, among others, be the
    // suggested filename in 'download' attribute of an anchor. Details:
    // http://www.whatwg.org/specs/web-apps/current-work/#downloading-hyperlinks
    std::string suggested_filename;

    // If non-empty, contains an externally supplied path that should be used as
    // the target path.
    base::FilePath forced_file_path;

    // Page transition that triggerred the download.
    ui::PageTransition transition_type = ui::PAGE_TRANSITION_LINK;

    // Whether the download was triggered with a user gesture.
    bool has_user_gesture = false;

    // The remote IP address where the download was fetched from.
    std::string remote_address;

    // Time the download was started.
    base::Time start_time;

    // True if the item was downloaded temporarily.
    // TODO(asanka): Get rid of the concept of temporary files. crbug.com/628329
    bool is_temporary = false;
  };

  struct ResponseInfo {
    // Information from the request.
    // Content-disposition field from the header.
    std::string content_disposition;

    // Mime-type from the header.  Subject to change.
    std::string mime_type;

    // The value of the content type header sent with the downloaded item.  It
    // may be different from |mime_type|, which may be set based on heuristics
    // which may look at the file extension and first few bytes of the file.
    std::string original_mime_type;

    // Contents of the Last-Modified header for the most recent server response.
    std::string last_modified_time;

    // Server's ETAG for the file.
    std::string etag;

    // Total bytes expected.
    int64_t total_bytes = 0;
  };

  // The following fields describe the current state of the download file if
  // any.
  struct DestinationInfo {
    // Display name for the destination. If this is empty, then the display name
    // is considered to be |target_path.BaseName()|.
    base::FilePath display_name;

    // Whether the target should be overwritten, uniquified or prompted for.
    TargetDisposition target_disposition = TARGET_DISPOSITION_OVERWRITE;

    // Target path of an in-progress download. We may be downloading to a
    // temporary or intermediate file (specified by |current_path|.  Once the
    // download completes, we will rename the file to |target_path|.
    base::FilePath target_path;

    // Full path to the downloaded or downloading file. This is the path to the
    // physical file, if one exists. The final target path is specified by
    // |target_path|. |current_path| can be empty if the in-progress path
    // hasn't been determined.
    base::FilePath current_path;

    // Current received bytes.
    int64_t received_bytes = 0;

    // True if we've saved all the data for the download. If true, then the file
    // at |current_path| contains |received_bytes|, which constitute the
    // entirety of what we expect to save there. A digest of its contents can be
    // found at |hash|.
    bool all_data_saved = false;

    // SHA256 hash of the possibly partial content. The hash is updated each
    // time the download is interrupted, and when the all the data has been
    // transferred. |hash| contains the raw binary hash and is not hex encoded.
    //
    // While the download is in progress, and while resuming, |hash| will be
    // empty.
    std::string hash;

    // Time last update was written to target file.
    base::Time end_time;
  };

  // Callback used with AcquireFileAndDeleteDownload().
  typedef base::Callback<void(const base::FilePath&)> AcquireFileCallback;

  // Used to represent an invalid download ID.
  static const uint32_t kInvalidId;

  // Interface that observers of a particular download must implement in order
  // to receive updates to the download's status.
  class CONTENT_EXPORT Observer {
   public:
    // Called after some observable change is made to the DownloadItem.
    virtual void OnDownloadUpdated(DownloadItem* download) {}

    // The download was opened via a call to Open().
    virtual void OnDownloadOpened(DownloadItem* download) {}

    // Called when Remove() is called. Signals that the download is to be
    // permanently removed. I.e. any persisted data about the download should be
    // removed when this notification is received.
    virtual void OnDownloadRemoved(DownloadItem* download) {}

    // Called when the download is being destroyed. This happens after
    // every OnDownloadRemoved() as well as when the DownloadManager is going
    // down.
    virtual void OnDownloadDestroyed(DownloadItem* download) {}

   protected:
    virtual ~Observer() {}
  };

  DownloadItem(const std::string& guid,
               uint32_t download_id,
               RequestInfo request,
               DownloadItemDelegate* delegate);

  ~DownloadItem() override;

  // Identity ------------------------------------------------------------------

  // Retrieve the ID for this download. The ID is provided by the owner of the
  // DownloadItem and is expected to uniquely identify the download within the
  // context of its container during the lifetime of the download. A valid
  // download will never return |kInvalidId|.
  virtual uint32_t GetId() const;

  // Retrieve the GUID for this download. The returned string is never empty and
  // will satisfy base::IsValidGUID(), and uniquely identifies the download
  // during its lifetime.
  virtual const std::string& GetGuid() const;

  // Observation ---------------------------------------------------------------

  virtual void AddObserver(DownloadItem::Observer* observer);
  virtual void RemoveObserver(DownloadItem::Observer* observer);

  // Invoke OnDownloadUpdated() on all registered Observers. Must be called
  // after making an observable change to the DownloadItem.
  virtual void UpdateObservers();

  // Job Control ---------------------------------------------------------------

  // Sets the job that controls the download actions.
  //
  // Invokes OnBeforeDetach() on the current job and OnAttached() on the new
  // job. Returns the previous job.
  std::unique_ptr<DownloadJob> AssignJob(std::unique_ptr<DownloadJob> job);

  // User Actions --------------------------------------------------------------

  // Called when the user has validated the download of a dangerous file.
  virtual void ValidateDangerousDownload();

  // Called to acquire a dangerous download. If |delete_file_afterward| is true,
  // invokes |callback| on the UI thread with the path to the downloaded file,
  // and removes the DownloadItem from views and history if appropriate.
  // Otherwise, makes a temp copy of the download file, and invokes |callback|
  // with the path to the temp copy. The caller is responsible for cleanup.
  // Note: It is important for |callback| to be valid since the downloaded file
  // will not be cleaned up if the callback fails.
  virtual void StealDangerousDownload(bool delete_file_afterward,
                                      const AcquireFileCallback& callback);

  // Pause a download.  Will have no effect if the download is already
  // paused.
  virtual void Pause();

  // Resume a download that has been paused or interrupted. Will have no effect
  // if the download is neither. Only does something if CanResume() returns
  // true.
  virtual void Resume();

  // Cancel the download operation.
  //
  // Set |user_cancel| to true if the cancellation was triggered by an explicit
  // user action. Non-user-initiated cancels typically happen when the browser
  // is being closed with in-progress downloads.
  virtual void Cancel(bool user_cancel);

  // Removes the download from the views and history. If the download was
  // in-progress or interrupted, then the intermediate file will also be
  // deleted.
  virtual void Remove();

  // Open the file associated with this download.  If the download is
  // still in progress, marks the download to be opened when it is complete.
  virtual void OpenDownload();

  // Show the download via the OS shell.
  virtual void ShowDownloadInShell();

  // State accessors -----------------------------------------------------------

  // Get the current state of the download. See DownloadState for descriptions
  // of each download state.
  virtual DownloadState GetState() const;

  // Returns the most recent interrupt reason for this download. Returns
  // DOWNLOAD_INTERRUPT_REASON_NONE if there is no previous interrupt reason.
  // Cancelled downloads return DOWNLOAD_INTERRUPT_REASON_USER_CANCELLED. If
  // the download was resumed, then the return value is the interrupt reason
  // prior to resumption.
  virtual DownloadInterruptReason GetLastReason() const;

  // The download is currently paused. Calling Resume() will transition out of
  // this paused state.
  virtual bool IsPaused() const;

  // DEPRECATED. True if this is a temporary download and should not be
  // persisted.
  virtual bool IsTemporary() const;

  // Returns true if the download can be resumed. A download can be resumed if
  // an in-progress download was paused or if an interrupted download requires
  // user-interaction to resume.
  virtual bool CanResume() const;

  // Returns true if the download is in a terminal state. This includes
  // completed downloads, cancelled downloads, and interrupted downloads that
  // can't be resumed.
  virtual bool IsDone() const;

  //    Origin State accessors -------------------------------------------------

  // Final URL. The primary resource being downloaded is from this URL. This is
  // the tail of GetUrlChain(). May return an empty GURL if there is no valid
  // download URL.
  virtual const GURL& GetURL() const;

  // The complete URL chain including redirects. URL at index i redirected to
  // URL at index i+1.
  virtual const std::vector<GURL>& GetUrlChain() const;

  // The URL that the download request originally attempted to fetch. This may
  // differ from GetURL() if there were redirects. The return value from this
  // accessor is the same as the head of GetUrlChain().
  virtual const GURL& GetOriginalUrl() const;

  // URL of document that is considered the referrer for the original URL.
  virtual const GURL& GetReferrerUrl() const;

  // Site instance URL. Used to locate the correct storage partition during
  // subsequent browser sessions. This may be different from all of
  // GetOriginalUrl(), GetURL() and GetReferrerUrl().
  virtual const GURL& GetSiteUrl() const;

  // URL of the top level frame at the time the download was initiated.
  virtual const GURL& GetTabUrl() const;

  // Referrer URL for top level frame.
  virtual const GURL& GetTabReferrerUrl() const;

  // For downloads initiated via <a download>, this is the suggested download
  // filename from the download attribute.
  virtual std::string GetSuggestedFilename() const;

  // Content-Disposition header value from HTTP response.
  virtual std::string GetContentDisposition() const;

  // Effective MIME type of downloaded content.
  virtual std::string GetMimeType() const;

  // Content-Type header value from HTTP response. May be different from
  // GetMimeType() if a different effective MIME type was chosen after MIME
  // sniffing.
  virtual std::string GetOriginalMimeType() const;

  // Remote address of server serving download contents.
  virtual std::string GetRemoteAddress() const;

  // Whether the download request was initiated in response to a user gesture.
  virtual bool HasUserGesture() const;

  // The page transition type associated with the download request.
  virtual ui::PageTransition GetTransitionType() const;

  // Last-Modified header value.
  virtual const std::string& GetLastModifiedTime() const;

  // ETag header value.
  virtual const std::string& GetETag() const;

  // Whether this download is a SavePackage download.
  virtual bool IsSavePackageDownload() const;

  //    Destination State accessors --------------------------------------------

  // Full path to the downloaded or downloading file. This is the path to the
  // physical file, if one exists. It should be considered a hint; changes to
  // this value and renames of the file on disk are not atomic with each other.
  // May be empty if the in-progress path hasn't been determined yet or if the
  // download was interrupted.
  //
  // DO NOT USE THIS METHOD to access the target path of the DownloadItem. Use
  // GetTargetFilePath() instead. While the download is in progress, the
  // intermediate file named by GetFullPath() may be renamed or disappear
  // completely on the FILE thread. The path may also be reset to empty when the
  // download is interrupted.
  virtual const base::FilePath& GetFullPath() const;

  // Target path of an in-progress download. We may be downloading to a
  // temporary or intermediate file (specified by GetFullPath()); this is the
  // name we will use once the download completes.
  // May be empty if the target path hasn't yet been determined.
  virtual const base::FilePath& GetTargetFilePath() const;

  // If the download forced a path rather than requesting name determination,
  // return the path requested.
  virtual const base::FilePath& GetForcedFilePath() const;

  // Returns the file-name that should be reported to the user. If a display
  // name has been explicitly set using SetDisplayName(), this function returns
  // that display name. Otherwise returns the final target filename.
  virtual base::FilePath GetFileNameToReportUser() const;

  // See TargetDisposition.
  virtual TargetDisposition GetTargetDisposition() const;

  // Final hash of completely downloaded file, or partial hash of an interrupted
  // download; only valid if GetState() == COMPLETED or INTERRUPTED. If
  // non-empty the returned string contains a raw SHA-256 hash (i.e. not hex
  // encoded).
  virtual const std::string& GetHash() const;

  // True if the file associated with the download has been removed by
  // external action.
  virtual bool GetFileExternallyRemoved() const;

  // If the file is successfully deleted, then GetFileExternallyRemoved() will
  // become true, GetFullPath() will become empty, and
  // DownloadItem::OnDownloadUpdated() will be called. Does nothing if
  // GetState() == COMPLETE or GetFileExternallyRemoved() is already true or
  // GetFullPath() is already empty. The callback is always run, and it is
  // always run asynchronously. It will be passed true if the file is
  // successfully deleted or if GetFilePath() was already empty or if
  // GetFileExternallyRemoved() was already true. The callback will be passed
  // false if the DownloadItem was not yet complete or if the file could not be
  // deleted for any reason.
  virtual void DeleteFile(const base::Callback<void(bool)>& callback);

  // True if the file that will be written by the download is dangerous
  // and we will require a call to ValidateDangerousDownload() to complete.
  // False if the download is safe or that function has been called.
  virtual bool IsDangerous() const;

  // Why |safety_state_| is not SAFE.
  virtual DownloadDangerType GetDangerType() const;

  //    Progress State accessors -----------------------------------------------

  // Simple calculation of the amount of time remaining to completion. Fills
  // |*remaining| with the amount of time remaining if successful. Fails and
  // returns false if we do not have the number of bytes or the speed so can
  // not estimate.
  virtual bool TimeRemaining(base::TimeDelta* remaining) const;

  // Simple speed estimate in bytes/s
  virtual int64_t CurrentSpeed() const;

  // Rough percent complete. Returns -1 if progress is unknown. 100 if the
  // download is already complete.
  virtual int PercentComplete() const;

  // Returns true if this download has saved all of its data. A download may
  // have saved all its data but still be waiting for some other process to
  // complete before the download is considered complete. E.g. A dangerous
  // download needs to be accepted by the user before the file is renamed to its
  // final name.
  virtual bool AllDataSaved() const;

  // Total number of expected bytes. Returns -1 if the total size is unknown.
  virtual int64_t GetTotalBytes() const;

  // Total number of bytes that have been received and written to the download
  // file.
  virtual int64_t GetReceivedBytes() const;

  // Time the download was first started. This timestamp is always valid and
  // doesn't change.
  virtual base::Time GetStartTime() const;

  // Time the download was marked as complete. Returns base::Time() if the
  // download hasn't reached a completion state yet.
  virtual base::Time GetEndTime() const;

  //    Open/Show State accessors ----------------------------------------------
  // TODO(asanka): Opening behavior should be moved out to the embedder.
  // crbug.com/167702

  // Returns true if it is OK to open a folder which this file is inside.
  virtual bool CanShowInFolder();

  // Returns true if it is OK to open the download.
  virtual bool CanOpenDownload();

  // Tests if a file type should be opened automatically.
  virtual bool ShouldOpenFileBasedOnExtension();

  // Returns true if the download will be auto-opened when complete.
  virtual bool GetOpenWhenComplete() const;

  // Returns true if the download has been auto-opened by the system.
  virtual bool GetAutoOpened() const;

  // Returns true if the download has been opened.
  virtual bool GetOpened() const;

  // Mark the download to be auto-opened when completed.
  virtual void SetOpenWhenComplete(bool open);

  //    Misc State accessors ---------------------------------------------------

  // BrowserContext that indirectly owns this download. Always valid.
  virtual BrowserContext* GetBrowserContext() const;

  // WebContents associated with the download. Returns nullptr if the
  // WebContents is unknown or if the download was not performed on behalf of a
  // renderer.
  virtual WebContents* GetWebContents() const;

  // External state transitions/setters ----------------------------------------

  // Called if a check of the download contents was performed and the results of
  // the test are available. This should only be called after AllDataSaved() is
  // true.
  virtual void OnContentCheckCompleted(DownloadDangerType danger_type);

  // Set a display name for the download that will be independent of the target
  // filename. If |name| is not empty, then GetFileNameToReportUser() will
  // return |name|. Has no effect on the final target filename.
  virtual void SetDisplayName(const base::FilePath& name);

  // Debug/testing -------------------------------------------------------------
  virtual std::string DebugString(bool verbose) const;

 private:
  friend class DownloadJob;

  // Identity.
  const std::string guid_;

  // TODO(asanka): Get rid of download ID. http://crbug.com/593020
  const uint32_t download_id_ = kInvalidId;

  DownloadItemDelegate* const delegate_ = nullptr;

  RequestInfo request_;
  ResponseInfo response_;
  DestinationInfo destination_;

  // DownloadJob if this download is active.
  std::unique_ptr<DownloadJob> job_;

  // State of the download.
  DownloadState state_ = DETACHED;

  // Last reason. Only considered valid if state is
  DownloadInterruptReason last_reason_ = DOWNLOAD_INTERRUPT_REASON_NONE;

  // Current danger type for the download.
  // TODO(asanka): Concept of dangerous downloads should be moved out to the
  // embedder. crbug.com/628331.
  DownloadDangerType danger_type_ = DOWNLOAD_DANGER_TYPE_NOT_DANGEROUS;

  // A flag for indicating if the download should be opened at completion.
  // TODO(asanka): Opening behavior should be moved out to the embedder.
  // crbug.com/167702.
  bool open_when_complete_ = false;

  // A flag for indicating if the downloaded file is externally removed.
  // TODO(asanka): To be moved out to the renderer.
  bool file_externally_removed_ = false;

  // True if the download was auto-opened. We set this rather than using
  // an observer as it's frequently possible for the download to be auto opened
  // before the observer is added.
  // TODO(asanka): Opening behavior should be moved out to the embedder.
  // crbug.com/167702
  bool auto_opened_ = false;

  // Did the user open the item either directly or indirectly (such as by
  // setting always open files of this type)? The shelf also sets this field
  // when the user closes the shelf before the item has been opened but should
  // be treated as though the user opened it.
  // TODO(asanka): Opening behavior should be moved out to the embedder.
  // crbug.com/167702
  bool opened_ = false;

  // The views of this item in the download shelf and download contents.
  base::ObserverList<Observer> observers_;

  DISALLOW_COPY_AND_ASSIGN(DownloadItem);
};

}  // namespace content

#endif  // CONTENT_PUBLIC_BROWSER_DOWNLOAD_ITEM_H_
