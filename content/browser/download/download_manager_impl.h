// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef CONTENT_BROWSER_DOWNLOAD_DOWNLOAD_MANAGER_IMPL_H_
#define CONTENT_BROWSER_DOWNLOAD_DOWNLOAD_MANAGER_IMPL_H_
#pragma once

#include "content/browser/download/download_manager.h"

#include <map>
#include <set>

#include "base/hash_tables.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/synchronization/lock.h"
#include "content/browser/download/download_status_updater_delegate.h"
#include "content/common/content_export.h"

class DownloadIdFactory;
class DownloadStatusUpdater;

class CONTENT_EXPORT DownloadManagerImpl
    : public DownloadManager,
      public DownloadStatusUpdaterDelegate {
 public:
  DownloadManagerImpl(content::DownloadManagerDelegate* delegate,
                  DownloadIdFactory* id_factory,
                  DownloadStatusUpdater* status_updater);

  // DownloadManager functions.
  virtual void Shutdown() OVERRIDE;
  virtual void GetTemporaryDownloads(const FilePath& dir_path,
                                     DownloadVector* result) OVERRIDE;
  virtual void GetAllDownloads(const FilePath& dir_path,
                               DownloadVector* result) OVERRIDE;
  virtual void SearchDownloads(const string16& query,
                               DownloadVector* result) OVERRIDE;
  virtual bool Init(content::BrowserContext* browser_context) OVERRIDE;
  virtual void StartDownload(int32 id) OVERRIDE;
  virtual void UpdateDownload(int32 download_id, int64 bytes_so_far,
                              int64 bytes_per_sec) OVERRIDE;
  virtual void OnResponseCompleted(int32 download_id, int64 size,
                                   const std::string& hash) OVERRIDE;
  virtual void CancelDownload(int32 download_id) OVERRIDE;
  virtual void OnDownloadInterrupted(int32 download_id, int64 size,
                                     InterruptReason reason) OVERRIDE;
  virtual void DownloadCancelledInternal(DownloadItem* download) OVERRIDE;
  virtual void RemoveDownload(int64 download_handle) OVERRIDE;
  virtual bool IsDownloadReadyForCompletion(DownloadItem* download) OVERRIDE;
  virtual void MaybeCompleteDownload(DownloadItem* download) OVERRIDE;
  virtual void OnDownloadRenamedToFinalName(int download_id,
                                            const FilePath& full_path,
                                            int uniquifier) OVERRIDE;
  virtual int RemoveDownloadsBetween(const base::Time remove_begin,
                                     const base::Time remove_end) OVERRIDE;
  virtual int RemoveDownloads(const base::Time remove_begin) OVERRIDE;
  virtual int RemoveAllDownloads() OVERRIDE;
  virtual void DownloadCompleted(int32 download_id) OVERRIDE;
  virtual void DownloadUrl(const GURL& url,
                           const GURL& referrer,
                           const std::string& referrer_encoding,
                           TabContents* tab_contents) OVERRIDE;
  virtual void DownloadUrlToFile(const GURL& url,
                                 const GURL& referrer,
                                 const std::string& referrer_encoding,
                                 const DownloadSaveInfo& save_info,
                                 TabContents* tab_contents) OVERRIDE;
  virtual void AddObserver(Observer* observer) OVERRIDE;
  virtual void RemoveObserver(Observer* observer) OVERRIDE;
  virtual void OnPersistentStoreQueryComplete(
      std::vector<DownloadPersistentStoreInfo>* entries) OVERRIDE;
  virtual void OnItemAddedToPersistentStore(int32 download_id,
                                            int64 db_handle) OVERRIDE;
  virtual void ShowDownloadInBrowser(DownloadItem* download) OVERRIDE;
  virtual int InProgressCount() const OVERRIDE;
  virtual content::BrowserContext* BrowserContext() OVERRIDE;
  virtual FilePath LastDownloadPath() OVERRIDE;
  virtual void CreateDownloadItem(
      DownloadCreateInfo* info,
      const DownloadRequestHandle& request_handle) OVERRIDE;
  virtual void ClearLastDownloadPath() OVERRIDE;
  virtual void FileSelected(const FilePath& path, void* params) OVERRIDE;
  virtual void FileSelectionCanceled(void* params) OVERRIDE;
  virtual void RestartDownload(int32 download_id) OVERRIDE;
  virtual void MarkDownloadOpened(DownloadItem* download) OVERRIDE;
  virtual void CheckForHistoryFilesRemoval() OVERRIDE;
  virtual void CheckForFileRemoval(DownloadItem* download_item) OVERRIDE;
  virtual void AssertQueueStateConsistent(DownloadItem* download) OVERRIDE;
  virtual DownloadItem* GetDownloadItem(int id) OVERRIDE;
  virtual void SavePageDownloadStarted(DownloadItem* download) OVERRIDE;
  virtual void SavePageDownloadFinished(DownloadItem* download) OVERRIDE;
  virtual DownloadItem* GetActiveDownloadItem(int id) OVERRIDE;
  virtual content::DownloadManagerDelegate* delegate() const OVERRIDE;
  virtual void SetDownloadManagerDelegate(
      content::DownloadManagerDelegate* delegate) OVERRIDE;
  virtual DownloadId GetNextId() OVERRIDE;

  // Overridden from DownloadStatusUpdaterDelegate:
  virtual bool IsDownloadProgressKnown() const OVERRIDE;
  virtual int64 GetInProgressDownloadCount() const OVERRIDE;
  virtual int64 GetReceivedDownloadBytes() const OVERRIDE;
  virtual int64 GetTotalDownloadBytes() const OVERRIDE;

 private:
  typedef std::set<DownloadItem*> DownloadSet;
  typedef base::hash_map<int64, DownloadItem*> DownloadMap;

  // For testing.
  friend class DownloadManagerTest;
  friend class DownloadTest;
  friend class MockDownloadManager;

  friend class base::RefCountedThreadSafe<
      DownloadManagerImpl, content::BrowserThread::DeleteOnUIThread>;
  friend struct content::BrowserThread::DeleteOnThread<
      content::BrowserThread::UI>;
  friend class DeleteTask<DownloadManagerImpl>;

  virtual ~DownloadManagerImpl();

  // Called on the FILE thread to check the existence of a downloaded file.
  void CheckForFileRemovalOnFileThread(int64 db_handle, const FilePath& path);

  // Called on the UI thread if the FILE thread detects the removal of
  // the downloaded file. The UI thread updates the state of the file
  // and then notifies this update to the file's observer.
  void OnFileRemovalDetected(int64 db_handle);

  // Called back after a target path for the file to be downloaded to has been
  // determined, either automatically based on the suggested file name, or by
  // the user in a Save As dialog box.
  virtual void ContinueDownloadWithPath(DownloadItem* download,
                                        const FilePath& chosen_file) OVERRIDE;

  // Retrieves the download from the |download_id|.
  // Returns NULL if the download is not active.
  virtual DownloadItem* GetActiveDownload(int32 download_id) OVERRIDE;

  // Removes |download| from the active and in progress maps.
  // Called when the download is cancelled or has an error.
  // Does nothing if the download is not in the history DB.
  void RemoveFromActiveList(DownloadItem* download);

  // Updates the delegate about the overall download progress.
  void UpdateDownloadProgress();

  // Inform observers that the model has changed.
  void NotifyModelChanged();

  // Debugging routine to confirm relationship between below
  // containers; no-op if NDEBUG.
  void AssertContainersConsistent() const;

  // Add a DownloadItem to history_downloads_.
  void AddDownloadItemToHistory(DownloadItem* item, int64 db_handle);

  // Remove from internal maps.
  int RemoveDownloadItems(const DownloadVector& pending_deletes);

  // Called when a download entry is committed to the persistent store.
  void OnDownloadItemAddedToPersistentStore(int32 download_id, int64 db_handle);

  // Called when Save Page As entry is committed to the persistent store.
  void OnSavePageItemAddedToPersistentStore(int32 download_id, int64 db_handle);

  // For unit tests only.
  virtual void SetFileManager(DownloadFileManager* file_manager) OVERRIDE;

  // |downloads_| is the owning set for all downloads known to the
  // DownloadManager.  This includes downloads started by the user in
  // this session, downloads initialized from the history system, and
  // "save page as" downloads.  All other DownloadItem containers in
  // the DownloadManager are maps; they do not own the DownloadItems.
  // Note that this is the only place (with any functional implications;
  // see save_page_downloads_ below) that "save page as" downloads are
  // kept, as the DownloadManager's only job is to hold onto those
  // until destruction.
  //
  // |history_downloads_| is map of all downloads in this browser context. The
  // key is the handle returned by the history system, which is unique across
  // sessions.
  //
  // |active_downloads_| is a map of all downloads that are currently being
  // processed. The key is the ID assigned by the DownloadFileManager,
  // which is unique for the current session.
  //
  // |in_progress_| is a map of all downloads that are in progress and that have
  // not yet received a valid history handle. The key is the ID assigned by the
  // DownloadFileManager, which is unique for the current session.
  //
  // |save_page_downloads_| (if defined) is a collection of all the
  // downloads the "save page as" system has given to us to hold onto
  // until we are destroyed. They key is DownloadFileManager, so it is unique
  // compared to download item. It is only used for debugging.
  //
  // When a download is created through a user action, the corresponding
  // DownloadItem* is placed in |active_downloads_| and remains there until the
  // download is in a terminal state (COMPLETE or CANCELLED).  It is also
  // placed in |in_progress_| and remains there until it has received a
  // valid handle from the history system. Once it has a valid handle, the
  // DownloadItem* is placed in the |history_downloads_| map.  When the
  // download reaches a terminal state, it is removed from |in_progress_|.
  // Downloads from past sessions read from a persisted state from the
  // history system are placed directly into |history_downloads_| since
  // they have valid handles in the history system.

  DownloadSet downloads_;
  DownloadMap history_downloads_;
  DownloadMap in_progress_;
  DownloadMap active_downloads_;
  DownloadMap save_page_downloads_;

  // True if the download manager has been initialized and requires a shutdown.
  bool shutdown_needed_;

  // Observers that want to be notified of changes to the set of downloads.
  ObserverList<Observer> observers_;

  // The current active browser context.
  content::BrowserContext* browser_context_;

  // Non-owning pointer for handling file writing on the download_thread_.
  DownloadFileManager* file_manager_;

  // Non-owning pointer for updating the download status.
  base::WeakPtr<DownloadStatusUpdater> status_updater_;

  // The user's last choice for download directory. This is only used when the
  // user wants us to prompt for a save location for each download.
  FilePath last_download_path_;

  // Allows an embedder to control behavior. Guaranteed to outlive this object.
  content::DownloadManagerDelegate* delegate_;

  DownloadIdFactory* id_factory_;

  // TODO(rdsmith): Remove when http://crbug.com/85408 is fixed.
  // For debugging only.
  int64 largest_db_handle_in_history_;

  DISALLOW_COPY_AND_ASSIGN(DownloadManagerImpl);
};

#endif  // CONTENT_BROWSER_DOWNLOAD_DOWNLOAD_MANAGER_IMPL_H_
