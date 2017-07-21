// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_HISTORY_CORE_BROWSER_HISTORY_SERVICE_IMPL_H_
#define COMPONENTS_HISTORY_CORE_BROWSER_HISTORY_SERVICE_IMPL_H_

#include <stddef.h>
#include <stdint.h>

#include <memory>
#include <set>
#include <string>
#include <vector>

#include "base/threading/thread_checker.h"
#include "build/build_config.h"
#include "components/history/core/browser/delete_directive_handler.h"
#include "components/history/core/browser/history_service.h"

class TestingProfile;

namespace base {
class FilePath;
class Thread;
}  // namespace base

namespace history {

class HistoryClient;
class InMemoryHistoryBackend;
class VisitDelegate;

class HistoryServiceImpl : public HistoryService {
 public:
  HistoryServiceImpl();
  HistoryServiceImpl(std::unique_ptr<HistoryClient> history_client,
                     std::unique_ptr<VisitDelegate> visit_delegate);
  ~HistoryServiceImpl() override;

  // HistoryService implementation.
  bool Init(const HistoryDatabaseParams& history_database_params) override;
  bool BackendLoaded() override;
#if defined(OS_IOS)
  void HandleBackgrounding() override;
#endif
  void ClearCachedDataForContextID(ContextID context_id) override;
  URLDatabase* InMemoryDatabase() override;
  TypedURLSyncBridge* GetTypedURLSyncBridge() const override;
  TypedUrlSyncableService* GetTypedUrlSyncableService() const override;
  void TopHosts(size_t num_hosts,
                const TopHostsCallback& callback) const override;
  void GetCountsAndLastVisitForOrigins(
      const std::set<GURL>& origins,
      const GetCountsAndLastVisitForOriginsCallback& callback) const override;
  void HostRankIfAvailable(
      const GURL& url,
      const base::Callback<void(int)>& callback) const override;
  void AddPage(const GURL& url,
               base::Time time,
               ContextID context_id,
               int nav_entry_id,
               const GURL& referrer,
               const RedirectList& redirects,
               ui::PageTransition transition,
               VisitSource visit_source,
               bool did_replace_entry) override;
  void AddPage(const GURL& url,
               base::Time time,
               VisitSource visit_source) override;
  void AddPage(const HistoryAddPageArgs& add_page_args) override;
  void AddPageNoVisitForBookmark(const GURL& url,
                                 const base::string16& title) override;
  void SetPageTitle(const GURL& url, const base::string16& title) override;
  void UpdateWithPageEndTime(ContextID context_id,
                             int nav_entry_id,
                             const GURL& url,
                             base::Time end_ts) override;
  base::CancelableTaskTracker::TaskId QueryURL(
      const GURL& url,
      bool want_visits,
      QueryURLCallback callback,
      base::CancelableTaskTracker* tracker) override;
  base::CancelableTaskTracker::TaskId QueryHistory(
      const base::string16& text_query,
      const QueryOptions& options,
      const QueryHistoryCallback& callback,
      base::CancelableTaskTracker* tracker) override;
  base::CancelableTaskTracker::TaskId QueryRedirectsFrom(
      const GURL& from_url,
      const QueryRedirectsCallback& callback,
      base::CancelableTaskTracker* tracker) override;
  base::CancelableTaskTracker::TaskId QueryRedirectsTo(
      const GURL& to_url,
      const QueryRedirectsCallback& callback,
      base::CancelableTaskTracker* tracker) override;

  base::CancelableTaskTracker::TaskId GetVisibleVisitCountToHost(
      const GURL& url,
      const GetVisibleVisitCountToHostCallback& callback,
      base::CancelableTaskTracker* tracker) override;
  base::CancelableTaskTracker::TaskId QueryMostVisitedURLs(
      int result_count,
      int days_back,
      const QueryMostVisitedURLsCallback& callback,
      base::CancelableTaskTracker* tracker) override;
  base::CancelableTaskTracker::TaskId GetHistoryCount(
      const base::Time& begin_time,
      const base::Time& end_time,
      const GetHistoryCountCallback& callback,
      base::CancelableTaskTracker* tracker) override;
  void DeleteURL(const GURL& url) override;
  void DeleteURLsForTest(const std::vector<GURL>& urls) override;
  void ExpireHistoryBetween(const std::set<GURL>& restrict_urls,
                            base::Time begin_time,
                            base::Time end_time,
                            const base::Closure& callback,
                            base::CancelableTaskTracker* tracker) override;
  void ExpireHistory(const std::vector<ExpireHistoryArgs>& expire_list,
                     const base::Closure& callback,
                     base::CancelableTaskTracker* tracker) override;
  void ExpireLocalAndRemoteHistoryBetween(
      WebHistoryService* web_history,
      const std::set<GURL>& restrict_urls,
      base::Time begin_time,
      base::Time end_time,
      const base::Closure& callback,
      base::CancelableTaskTracker* tracker) override;
  syncer::SyncError ProcessLocalDeleteDirective(
      const sync_pb::HistoryDeleteDirectiveSpecifics& delete_directive)
      override;
  void CreateDownload(const DownloadRow& info,
                      const DownloadCreateCallback& callback) override;
  void GetNextDownloadId(const DownloadIdCallback& callback) override;
  void QueryDownloads(const DownloadQueryCallback& callback) override;
  void UpdateDownload(const DownloadRow& data,
                      bool should_commit_immediately) override;
  void RemoveDownloads(const std::set<uint32_t>& ids) override;
  void SetKeywordSearchTermsForURL(const GURL& url,
                                   KeywordID keyword_id,
                                   const base::string16& term) override;
  void DeleteAllSearchTermsForKeyword(KeywordID keyword_id) override;
  void DeleteKeywordSearchTermForURL(const GURL& url) override;
  void DeleteMatchingURLsForKeyword(KeywordID keyword_id,
                                    const base::string16& term) override;
  void URLsNoLongerBookmarked(const std::set<GURL>& urls) override;
  void AddObserver(HistoryServiceObserver* observer) override;
  void RemoveObserver(HistoryServiceObserver* observer) override;
  base::CancelableTaskTracker::TaskId ScheduleDBTask(
      std::unique_ptr<HistoryDBTask> task,
      base::CancelableTaskTracker* tracker) override;
  std::unique_ptr<base::CallbackList<void(const std::set<GURL>&,
                                          const GURL&)>::Subscription>
  AddFaviconsChangedCallback(
      const OnFaviconsChangedCallback& callback) override;
  void FlushForTest(const base::Closure& flushed) override;
  void SetOnBackendDestroyTask(const base::Closure& task) override;
  void AddPageWithDetails(const GURL& url,
                          const base::string16& title,
                          int visit_count,
                          int typed_count,
                          base::Time last_visit,
                          bool hidden,
                          VisitSource visit_source) override;
  void AddPagesWithDetails(const URLRows& info,
                           VisitSource visit_source) override;
  base::WeakPtr<HistoryService> AsWeakPtr() override;

  // KeyedService implementation.
  void Shutdown() override;

  // syncer::SyncableService implementation.
  syncer::SyncMergeResult MergeDataAndStartSyncing(
      syncer::ModelType type,
      const syncer::SyncDataList& initial_sync_data,
      std::unique_ptr<syncer::SyncChangeProcessor> sync_processor,
      std::unique_ptr<syncer::SyncErrorFactory> error_handler) override;
  void StopSyncing(syncer::ModelType type) override;
  syncer::SyncDataList GetAllSyncData(syncer::ModelType type) const override;
  syncer::SyncError ProcessSyncChanges(
      const tracked_objects::Location& from_here,
      const syncer::SyncChangeList& change_list) override;

 protected:
  // These are not currently used, hopefully we can do something in the future
  // to ensure that the most important things happen first.
  enum SchedulePriority {
    PRIORITY_UI,      // The highest priority (must respond to UI events).
    PRIORITY_NORMAL,  // Normal stuff like adding a page.
    PRIORITY_LOW,     // Low priority things like indexing or expiration.
  };

 private:
  class BackendDelegate;
  friend class BackendDelegate;
  friend class HistoryQueryTest;
  friend class HistoryServiceTest;
  friend class HQPPerfTestOnePopularURL;
  friend class ::TestingProfile;
  friend std::unique_ptr<HistoryService> CreateHistoryService(
      const base::FilePath& history_dir,
      bool create_db);

  // HistoryService implementation.
  bool Init(bool no_db,
            const HistoryDatabaseParams& history_database_params) override;
  void ScheduleAutocomplete(
      const base::Callback<void(HistoryBackend*, URLDatabase*)>& callback)
      override;
  HistoryBackend* GetHistoryBackend() override;
  base::CancelableTaskTracker::TaskId GetFavicon(
      const GURL& icon_url,
      favicon_base::IconType icon_type,
      const std::vector<int>& desired_sizes,
      const favicon_base::FaviconResultsCallback& callback,
      base::CancelableTaskTracker* tracker) override;
  base::CancelableTaskTracker::TaskId GetFaviconsForURL(
      const GURL& page_url,
      int icon_types,
      const std::vector<int>& desired_sizes,
      const favicon_base::FaviconResultsCallback& callback,
      base::CancelableTaskTracker* tracker) override;
  base::CancelableTaskTracker::TaskId GetLargestFaviconForURL(
      const GURL& page_url,
      const std::vector<int>& icon_types,
      int minimum_size_in_pixels,
      const favicon_base::FaviconRawBitmapCallback& callback,
      base::CancelableTaskTracker* tracker) override;
  base::CancelableTaskTracker::TaskId GetFaviconForID(
      favicon_base::FaviconID favicon_id,
      int desired_size,
      const favicon_base::FaviconResultsCallback& callback,
      base::CancelableTaskTracker* tracker) override;
  base::CancelableTaskTracker::TaskId UpdateFaviconMappingsAndFetch(
      const GURL& page_url,
      const GURL& icon_url,
      favicon_base::IconType icon_type,
      const std::vector<int>& desired_sizes,
      const favicon_base::FaviconResultsCallback& callback,
      base::CancelableTaskTracker* tracker) override;
  void MergeFavicon(const GURL& page_url,
                    const GURL& icon_url,
                    favicon_base::IconType icon_type,
                    scoped_refptr<base::RefCountedMemory> bitmap_data,
                    const gfx::Size& pixel_size) override;
  void SetFavicons(const GURL& page_url,
                   favicon_base::IconType icon_type,
                   const GURL& icon_url,
                   const std::vector<SkBitmap>& bitmaps) override;
  void SetOnDemandFavicons(const GURL& page_url,
                           favicon_base::IconType icon_type,
                           const GURL& icon_url,
                           const std::vector<SkBitmap>& bitmaps,
                           base::Callback<void(bool)> callback) override;
  void SetFaviconsOutOfDateForPage(const GURL& page_url) override;
  void TouchOnDemandFavicon(const GURL& icon_url) override;
  void SetImportedFavicons(
      const favicon_base::FaviconUsageDataList& favicon_usage) override;

  // Called on shutdown, this will tell the history backend to complete and
  // will release pointers to it. No other functions should be called once
  // cleanup has happened that may dispatch to the history thread (because it
  // will be null).
  //
  // In practice, this will be called by the service manager (BrowserProcess)
  // when it is being destroyed. Because that reference is being destroyed, it
  // should be impossible for anybody else to call the service, even if it is
  // still in memory (pending requests may be holding a reference to us).
  void Cleanup();

  // Notification from the backend that it has finished loading. Sends
  // notification (NOTIFY_HISTORY_LOADED) and sets backend_loaded_ to true.
  void OnDBLoaded();

  // Observers ----------------------------------------------------------------

  // Notify all HistoryServiceObservers registered that user is visiting a URL.
  // The |row| ID will be set to the value that is currently in effect in the
  // main history database. |redirects| is the list of redirects leading up to
  // the URL. If we have a redirect chain A -> B -> C and user is visiting C,
  // then |redirects[0]=B| and |redirects[1]=A|. If there are no redirects,
  // |redirects| is an empty vector.
  void NotifyURLVisited(ui::PageTransition transition,
                        const URLRow& row,
                        const RedirectList& redirects,
                        base::Time visit_time);

  // Notify all HistoryServiceObservers registered that URLs have been added or
  // modified. |changed_urls| contains the list of affects URLs.
  void NotifyURLsModified(const URLRows& changed_urls);

  // Notify all HistoryServiceObservers registered that URLs have been deleted.
  // |all_history| is set to true, if all the URLs are deleted.
  //               When set to true, |deleted_rows| and |favicon_urls| are
  //               undefined.
  // |expired| is set to true, if the URL deletion is due to expiration.
  // |deleted_rows| list of the deleted URLs.
  // |favicon_urls| list of favicon URLs that correspond to the deleted URLs.
  void NotifyURLsDeleted(bool all_history,
                         bool expired,
                         const URLRows& deleted_rows,
                         const std::set<GURL>& favicon_urls);

  // Notify all HistoryServiceObservers registered that the
  // HistoryService has finished loading.
  void NotifyHistoryServiceLoaded();

  // Notify all HistoryServiceObservers registered that HistoryService is being
  // deleted.
  void NotifyHistoryServiceBeingDeleted();

  // Notify all HistoryServiceObservers registered that a keyword search term
  // has been updated. |row| contains the URL information for search |term|.
  // |keyword_id| associated with a URL and search term.
  void NotifyKeywordSearchTermUpdated(const URLRow& row,
                                      KeywordID keyword_id,
                                      const base::string16& term);

  // Notify all HistoryServiceObservers registered that keyword search term is
  // deleted. |url_id| is the id of the url row.
  void NotifyKeywordSearchTermDeleted(URLID url_id);

  // Sets the in-memory URL database. This is called by the backend once the
  // database is loaded to make it available.
  void SetInMemoryBackend(std::unique_ptr<InMemoryHistoryBackend> mem_backend);

  // Called by our BackendDelegate when there is a problem reading the database.
  void NotifyProfileError(sql::InitStatus init_status,
                          const std::string& diagnostics);

  // Call to schedule a given task for running on the history thread with the
  // specified priority. The task will have ownership taken.
  void ScheduleTask(SchedulePriority priority, base::OnceClosure task);

  // Called when the favicons for the given page URLs (e.g.
  // http://www.google.com) and the given icon URL (e.g.
  // http://www.google.com/favicon.ico) have changed. It is valid to call
  // NotifyFaviconsChanged() with non-empty |page_urls| and an empty |icon_url|
  // and vice versa.
  void NotifyFaviconsChanged(const std::set<GURL>& page_urls,
                             const GURL& icon_url);

  base::ThreadChecker thread_checker_;

  // The thread used by the history service to run HistoryBackend operations.
  // Intentionally not a BrowserThread because the sync integration unit tests
  // need to create multiple HistoryServices which each have their own thread.
  // Nullptr if TaskScheduler is used for HistoryBackend operations.
  std::unique_ptr<base::Thread> thread_;

  // The TaskRunner to which HistoryBackend tasks are posted. Nullptr once
  // Cleanup() is called.
  scoped_refptr<base::SequencedTaskRunner> backend_task_runner_;

  // This class has most of the implementation and runs on the 'thread_'.
  // You MUST communicate with this class ONLY through the thread_'s
  // message_loop().
  //
  // This pointer will be null once Cleanup() has been called, meaning no
  // more calls should be made to the history thread.
  scoped_refptr<HistoryBackend> history_backend_;

  // A cache of the user-typed URLs kept in memory that is used by the
  // autocomplete system. This will be null until the database has been created
  // on the background thread.
  // TODO(mrossetti): Consider changing ownership. See http://crbug.com/138321
  std::unique_ptr<InMemoryHistoryBackend> in_memory_backend_;

  // The history client, may be null when testing.
  std::unique_ptr<HistoryClient> history_client_;

  // The history service will inform its VisitDelegate of URLs recorded and
  // removed from the history database. This may be null during testing.
  std::unique_ptr<VisitDelegate> visit_delegate_;

  // Has the backend finished loading? The backend is loaded once Init has
  // completed.
  bool backend_loaded_;

  base::ObserverList<HistoryServiceObserver> observers_;
  base::CallbackList<void(const std::set<GURL>&, const GURL&)>
      favicon_changed_callback_list_;

  DeleteDirectiveHandler delete_directive_handler_;

  // All vended weak pointers are invalidated in Cleanup().
  base::WeakPtrFactory<HistoryServiceImpl> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(HistoryServiceImpl);
};

}  // namespace history

#endif  // COMPONENTS_HISTORY_CORE_BROWSER_HISTORY_SERVICE_IMPL_H_
