// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_HISTORY_CORE_BROWSER_FAKE_HISTORY_SERVICE_H_
#define COMPONENTS_HISTORY_CORE_BROWSER_FAKE_HISTORY_SERVICE_H_

#include <memory>
#include <set>
#include <string>
#include <vector>

#include "build/build_config.h"
#include "components/history/core/browser/history_service.h"

namespace history {

class FakeHistoryService : public HistoryService {
 public:
  FakeHistoryService();
  ~FakeHistoryService() override;

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

 private:
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

  DISALLOW_COPY_AND_ASSIGN(FakeHistoryService);
};

}  // namespace history

#endif  // COMPONENTS_HISTORY_CORE_BROWSER_FAKE_HISTORY_SERVICE_H_
