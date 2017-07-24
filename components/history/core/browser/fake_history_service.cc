// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/history/core/browser/fake_history_service.h"

#include "components/history/core/browser/history_db_task.h"

using base::CancelableTaskTracker;

namespace history {

FakeHistoryService::FakeHistoryService() {}

FakeHistoryService::~FakeHistoryService() {}

bool FakeHistoryService::Init(
    const HistoryDatabaseParams& history_database_params) {
  return false;
}

bool FakeHistoryService::BackendLoaded() {
  return false;
}

#if defined(OS_IOS)
void FakeHistoryService::HandleBackgrounding() {}
#endif

void FakeHistoryService::ClearCachedDataForContextID(ContextID context_id) {}

URLDatabase* FakeHistoryService::InMemoryDatabase() {
  return nullptr;
}

TypedURLSyncBridge* FakeHistoryService::GetTypedURLSyncBridge() const {
  return nullptr;
}

TypedUrlSyncableService* FakeHistoryService::GetTypedUrlSyncableService()
    const {
  return nullptr;
}

void FakeHistoryService::TopHosts(size_t num_hosts,
                                  const TopHostsCallback& callback) const {}

void FakeHistoryService::GetCountsAndLastVisitForOrigins(
    const std::set<GURL>& origins,
    const GetCountsAndLastVisitForOriginsCallback& callback) const {}

void FakeHistoryService::HostRankIfAvailable(
    const GURL& url,
    const base::Callback<void(int)>& callback) const {}

void FakeHistoryService::AddPage(const GURL& url,
                                 base::Time time,
                                 ContextID context_id,
                                 int nav_entry_id,
                                 const GURL& referrer,
                                 const RedirectList& redirects,
                                 ui::PageTransition transition,
                                 VisitSource visit_source,
                                 bool did_replace_entry) {}

void FakeHistoryService::AddPage(const GURL& url,
                                 base::Time time,
                                 VisitSource visit_source) {}

void FakeHistoryService::AddPage(const HistoryAddPageArgs& add_page_args) {}

void FakeHistoryService::AddPageNoVisitForBookmark(
    const GURL& url,
    const base::string16& title) {}

void FakeHistoryService::SetPageTitle(const GURL& url,
                                      const base::string16& title) {}

void FakeHistoryService::UpdateWithPageEndTime(ContextID context_id,
                                               int nav_entry_id,
                                               const GURL& url,
                                               base::Time end_ts) {}

CancelableTaskTracker::TaskId FakeHistoryService::QueryURL(
    const GURL& url,
    bool want_visits,
    QueryURLCallback callback,
    CancelableTaskTracker* tracker) {
  return CancelableTaskTracker::kBadTaskId;
}

CancelableTaskTracker::TaskId FakeHistoryService::QueryHistory(
    const base::string16& text_query,
    const QueryOptions& options,
    const QueryHistoryCallback& callback,
    CancelableTaskTracker* tracker) {
  return CancelableTaskTracker::kBadTaskId;
}

CancelableTaskTracker::TaskId FakeHistoryService::QueryRedirectsFrom(
    const GURL& from_url,
    const QueryRedirectsCallback& callback,
    CancelableTaskTracker* tracker) {
  return CancelableTaskTracker::kBadTaskId;
}

CancelableTaskTracker::TaskId FakeHistoryService::QueryRedirectsTo(
    const GURL& to_url,
    const QueryRedirectsCallback& callback,
    CancelableTaskTracker* tracker) {
  return CancelableTaskTracker::kBadTaskId;
}

CancelableTaskTracker::TaskId FakeHistoryService::GetVisibleVisitCountToHost(
    const GURL& url,
    const GetVisibleVisitCountToHostCallback& callback,
    CancelableTaskTracker* tracker) {
  return CancelableTaskTracker::kBadTaskId;
}

CancelableTaskTracker::TaskId FakeHistoryService::QueryMostVisitedURLs(
    int result_count,
    int days_back,
    const QueryMostVisitedURLsCallback& callback,
    CancelableTaskTracker* tracker) {
  return CancelableTaskTracker::kBadTaskId;
}

CancelableTaskTracker::TaskId FakeHistoryService::GetHistoryCount(
    const base::Time& begin_time,
    const base::Time& end_time,
    const GetHistoryCountCallback& callback,
    CancelableTaskTracker* tracker) {
  return CancelableTaskTracker::kBadTaskId;
}

void FakeHistoryService::DeleteURL(const GURL& url) {}

void FakeHistoryService::DeleteURLsForTest(const std::vector<GURL>& urls) {}

void FakeHistoryService::ExpireHistoryBetween(
    const std::set<GURL>& restrict_urls,
    base::Time begin_time,
    base::Time end_time,
    const base::Closure& callback,
    CancelableTaskTracker* tracker) {}

void FakeHistoryService::ExpireHistory(
    const std::vector<ExpireHistoryArgs>& expire_list,
    const base::Closure& callback,
    CancelableTaskTracker* tracker) {}

void FakeHistoryService::ExpireLocalAndRemoteHistoryBetween(
    WebHistoryService* web_history,
    const std::set<GURL>& restrict_urls,
    base::Time begin_time,
    base::Time end_time,
    const base::Closure& callback,
    CancelableTaskTracker* tracker) {}

syncer::SyncError FakeHistoryService::ProcessLocalDeleteDirective(
    const sync_pb::HistoryDeleteDirectiveSpecifics& delete_directive) {
  return syncer::SyncError();
}

void FakeHistoryService::CreateDownload(
    const DownloadRow& info,
    const DownloadCreateCallback& callback) {}

void FakeHistoryService::GetNextDownloadId(const DownloadIdCallback& callback) {
}

void FakeHistoryService::QueryDownloads(const DownloadQueryCallback& callback) {
}

void FakeHistoryService::UpdateDownload(const DownloadRow& data,
                                        bool should_commit_immediately) {}

void FakeHistoryService::RemoveDownloads(const std::set<uint32_t>& ids) {}

void FakeHistoryService::SetKeywordSearchTermsForURL(
    const GURL& url,
    KeywordID keyword_id,
    const base::string16& term) {}

void FakeHistoryService::DeleteAllSearchTermsForKeyword(KeywordID keyword_id) {}

void FakeHistoryService::DeleteKeywordSearchTermForURL(const GURL& url) {}

void FakeHistoryService::DeleteMatchingURLsForKeyword(
    KeywordID keyword_id,
    const base::string16& term) {}

void FakeHistoryService::URLsNoLongerBookmarked(const std::set<GURL>& urls) {}

void FakeHistoryService::AddObserver(HistoryServiceObserver* observer) {}

void FakeHistoryService::RemoveObserver(HistoryServiceObserver* observer) {}

CancelableTaskTracker::TaskId FakeHistoryService::ScheduleDBTask(
    std::unique_ptr<HistoryDBTask> task,
    CancelableTaskTracker* tracker) {
  return CancelableTaskTracker::kBadTaskId;
}

std::unique_ptr<
    base::CallbackList<void(const std::set<GURL>&, const GURL&)>::Subscription>
FakeHistoryService::AddFaviconsChangedCallback(
    const OnFaviconsChangedCallback& callback) {
  return nullptr;
}

void FakeHistoryService::FlushForTest(const base::Closure& flushed) {}

void FakeHistoryService::SetOnBackendDestroyTask(const base::Closure& task) {}

void FakeHistoryService::AddPageWithDetails(const GURL& url,
                                            const base::string16& title,
                                            int visit_count,
                                            int typed_count,
                                            base::Time last_visit,
                                            bool hidden,
                                            VisitSource visit_source) {}

void FakeHistoryService::AddPagesWithDetails(const URLRows& info,
                                             VisitSource visit_source) {}

base::WeakPtr<HistoryService> FakeHistoryService::AsWeakPtr() {
  return nullptr;
}

void FakeHistoryService::Shutdown() {}

syncer::SyncMergeResult FakeHistoryService::MergeDataAndStartSyncing(
    syncer::ModelType type,
    const syncer::SyncDataList& initial_sync_data,
    std::unique_ptr<syncer::SyncChangeProcessor> sync_processor,
    std::unique_ptr<syncer::SyncErrorFactory> error_handler) {
  return syncer::SyncMergeResult(syncer::UNSPECIFIED);
}

void FakeHistoryService::StopSyncing(syncer::ModelType type) {}

syncer::SyncDataList FakeHistoryService::GetAllSyncData(
    syncer::ModelType type) const {
  return {};
}

syncer::SyncError FakeHistoryService::ProcessSyncChanges(
    const tracked_objects::Location& from_here,
    const syncer::SyncChangeList& change_list) {
  return syncer::SyncError();
}

bool FakeHistoryService::Init(
    bool no_db,
    const HistoryDatabaseParams& history_database_params) {
  return false;
}

void FakeHistoryService::ScheduleAutocomplete(
    const base::Callback<void(HistoryBackend*, URLDatabase*)>& callback) {}

HistoryBackend* FakeHistoryService::GetHistoryBackend() {
  return nullptr;
}

CancelableTaskTracker::TaskId FakeHistoryService::GetFavicon(
    const GURL& icon_url,
    favicon_base::IconType icon_type,
    const std::vector<int>& desired_sizes,
    const favicon_base::FaviconResultsCallback& callback,
    CancelableTaskTracker* tracker) {
  return CancelableTaskTracker::kBadTaskId;
}

CancelableTaskTracker::TaskId FakeHistoryService::GetFaviconsForURL(
    const GURL& page_url,
    int icon_types,
    const std::vector<int>& desired_sizes,
    const favicon_base::FaviconResultsCallback& callback,
    CancelableTaskTracker* tracker) {
  return CancelableTaskTracker::kBadTaskId;
}

CancelableTaskTracker::TaskId FakeHistoryService::GetLargestFaviconForURL(
    const GURL& page_url,
    const std::vector<int>& icon_types,
    int minimum_size_in_pixels,
    const favicon_base::FaviconRawBitmapCallback& callback,
    CancelableTaskTracker* tracker) {
  return CancelableTaskTracker::kBadTaskId;
}

CancelableTaskTracker::TaskId FakeHistoryService::GetFaviconForID(
    favicon_base::FaviconID favicon_id,
    int desired_size,
    const favicon_base::FaviconResultsCallback& callback,
    CancelableTaskTracker* tracker) {
  return CancelableTaskTracker::kBadTaskId;
}

CancelableTaskTracker::TaskId FakeHistoryService::UpdateFaviconMappingsAndFetch(
    const GURL& page_url,
    const GURL& icon_url,
    favicon_base::IconType icon_type,
    const std::vector<int>& desired_sizes,
    const favicon_base::FaviconResultsCallback& callback,
    CancelableTaskTracker* tracker) {
  return CancelableTaskTracker::kBadTaskId;
}

void FakeHistoryService::MergeFavicon(
    const GURL& page_url,
    const GURL& icon_url,
    favicon_base::IconType icon_type,
    scoped_refptr<base::RefCountedMemory> bitmap_data,
    const gfx::Size& pixel_size) {}

void FakeHistoryService::SetFavicons(const GURL& page_url,
                                     favicon_base::IconType icon_type,
                                     const GURL& icon_url,
                                     const std::vector<SkBitmap>& bitmaps) {}

void FakeHistoryService::SetOnDemandFavicons(
    const GURL& page_url,
    favicon_base::IconType icon_type,
    const GURL& icon_url,
    const std::vector<SkBitmap>& bitmaps,
    base::Callback<void(bool)> callback) {}

void FakeHistoryService::SetFaviconsOutOfDateForPage(const GURL& page_url) {}

void FakeHistoryService::TouchOnDemandFavicon(const GURL& icon_url) {}

void FakeHistoryService::SetImportedFavicons(
    const favicon_base::FaviconUsageDataList& favicon_usage) {}

}  // namespace history
