// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_HISTORY_CORE_BROWSER_BROWSING_HISTORY_SERVICE_H_
#define COMPONENTS_HISTORY_CORE_BROWSER_BROWSING_HISTORY_SERVICE_H_

#include <stdint.h>

#include <memory>
#include <set>
#include <string>
#include <vector>

#include "base/callback_forward.h"
#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/scoped_observer.h"
#include "base/strings/string16.h"
#include "base/task/cancelable_task_tracker.h"
#include "base/time/clock.h"
#include "base/timer/timer.h"
#include "base/values.h"
#include "components/history/core/browser/history_service_observer.h"
#include "components/history/core/browser/url_row.h"
#include "components/history/core/browser/web_history_service.h"
#include "components/history/core/browser/web_history_service_observer.h"
#include "components/sync/driver/sync_service_observer.h"
#include "url/gurl.h"

namespace syncer {
class SyncService;
class SyncServiceObserver;
}  // namespace syncer

FORWARD_DECLARE_TEST(BrowsingHistoryHandlerTest, ObservingWebHistoryDeletions);

namespace history {

class BrowsingHistoryDriver;
class HistoryService;
class QueryResults;
struct QueryOptions;

// Interacts with HistoryService, WebHistoryService, and SyncService to query
// history and provide results to the associated BrowsingHistoryDriver.
class BrowsingHistoryService : public HistoryServiceObserver,
                               public WebHistoryServiceObserver,
                               public syncer::SyncServiceObserver {
 public:
  // Represents a history entry to be shown to the user, representing either
  // a local or remote visit. A single entry can represent multiple visits,
  // since only the most recent visit on a particular day is shown.
  struct HistoryEntry {
    // Values indicating whether an entry represents only local visits, only
    // remote visits, or a mixture of both.
    enum EntryType {
      EMPTY_ENTRY = 0,
      LOCAL_ENTRY,
      REMOTE_ENTRY,
      COMBINED_ENTRY
    };

    HistoryEntry(EntryType type,
                 const GURL& url,
                 const base::string16& title,
                 base::Time time,
                 const std::string& client_id,
                 bool is_search_result,
                 const base::string16& snippet,
                 bool blocked_visit);
    HistoryEntry();
    HistoryEntry(const HistoryEntry& other);
    virtual ~HistoryEntry();

    // Comparison function for sorting HistoryEntries from newest to oldest.
    static bool SortByTimeDescending(const HistoryEntry& entry1,
                                     const HistoryEntry& entry2);

    // The type of visits this entry represents: local, remote, or both.
    EntryType entry_type;

    GURL url;
    base::string16 title;  // Title of the entry. May be empty.

    // The time of the entry. Usually this will be the time of the most recent
    // visit to |url| on a particular day as defined in the local timezone.
    base::Time time;

    // The sync ID of the client on which the most recent visit occurred.
    std::string client_id;

    // Timestamps of all local or remote visits the same URL on the same day.
    // TODO(skym): These should probably be converted to base::Time.
    std::set<int64_t> all_timestamps;

    // If true, this entry is a search result.
    bool is_search_result;

    // The entry's search snippet, if this entry is a search result.
    base::string16 snippet;

    // Whether this entry was blocked when it was attempted.
    bool blocked_visit;
  };

  // Contains information about a completed history query.
  struct QueryResultsInfo {
    ~QueryResultsInfo();

    // The query search text.
    base::string16 search_text;

    // Whether this query reached the end of all results, or if there are more
    // history entries that can be fetched through paging.
    bool reached_beginning = false;

    // TODO(skym): Can we consolidate |sync_timed_out| and |has_sync_results|?
    // Whether the last call to Web History timed out.
    bool sync_timed_out = false;

    // Whether the last call to Web History returned successfully with a message
    // body. During continuation queries we are not guaranteed to always make a
    // call to WebHistory, and this value could reflect the state from previous
    // queries.
    bool has_synced_results = false;
  };

  BrowsingHistoryService(BrowsingHistoryDriver* driver,
                         HistoryService* local_history,
                         syncer::SyncService* sync_service);
  ~BrowsingHistoryService() override;

  // Start a new query with the given parameters.
  void QueryHistory(const base::string16& search_text,
                    const QueryOptions& options);

  // Removes |items| from history.
  void RemoveVisits(const std::vector<HistoryEntry>& items);

  // SyncServiceObserver implementation.
  void OnStateChanged(syncer::SyncService* sync) override;

  // TODO(skym): Make these private;
  enum QuerySourceStatus {
    // Not a continuation and no response yet.
    UNINITIALIZED,
    // Could not query the particular source.
    NO_DEPENDENCY,
    // Only used for web, when we stop waiting for a response due to timeout.
    TIMED_OUT,
    // Only used for remote, response was error or empty.
    FAILURE,
    // Successfully retrieved results, but there are more left.
    MORE_RESULTS,
    // Successfully retrieved results and we reached the end of results.
    REACHED_BEGINNING,
  };

  struct QueryHistoryState {
    QueryHistoryState();
    QueryHistoryState(const QueryHistoryState&);
    ~QueryHistoryState();

    base::string16 search_text;
    QueryOptions original_options;

    QuerySourceStatus local_status = UNINITIALIZED;
    std::vector<HistoryEntry> local_results;
    base::Time local_end_time_for_continuation;

    QuerySourceStatus remote_status = UNINITIALIZED;
    std::vector<HistoryEntry> remote_results;
    base::Time remote_end_time_for_continuation;
  };

  // Merges duplicate entries from the query results, only retaining the most
  // recent visit to a URL on a particular day. That visit contains the
  // timestamps of the other visits.
  static void MergeDuplicateResults(QueryHistoryState* state,
                                    std::vector<HistoryEntry>* results);

  // Callback from the history system when a history query has completed.
  // Exposed for testing.
  void QueryComplete(QueryResults* results);

 private:
  FRIEND_TEST_ALL_PREFIXES(::BrowsingHistoryHandlerTest,
                           ObservingWebHistoryDeletions);

  // Core implementation of history querying.
  void QueryHistoryInternal(QueryHistoryState state);

  // Combines the query results from the local history database and the history
  // server, and sends the combined results to the
  // BrowsingHistoryDriver.
  void ReturnResultsToDriver();

  // Callback from |web_history_timer_| when a response from web history has
  // not been received in time.
  void WebHistoryTimeout();

  // Callback from the WebHistoryService when a query has completed.
  void WebHistoryQueryComplete(base::Time start_time,
                               WebHistoryService::Request* request,
                               const base::DictionaryValue* results_value);

  // Callback telling us whether other forms of browsing history were found
  // on the history server.
  void OtherFormsOfBrowsingHistoryQueryComplete(
      bool found_other_forms_of_browsing_history);

  // Callback from the history system when visits were deleted.
  void RemoveComplete();

  // Callback from history server when visits were deleted.
  void RemoveWebHistoryComplete(bool success);

  // HistoryServiceObserver implementation.
  void OnURLsDeleted(HistoryService* history_service,
                     bool all_history,
                     bool expired,
                     const URLRows& deleted_rows,
                     const std::set<GURL>& favicon_urls) override;

  // WebHistoryServiceObserver implementation.
  void OnWebHistoryDeleted() override;

  // Tracker for search requests to the history service.
  base::CancelableTaskTracker query_task_tracker_;

  // The currently-executing request for synced history results.
  // Deleting the request will cancel it.
  std::unique_ptr<WebHistoryService::Request> web_history_request_;

  // True if there is a pending delete requests to the history service.
  bool has_pending_delete_request_ = false;

  // Tracker for delete requests to the history service.
  base::CancelableTaskTracker delete_task_tracker_;

  // The list of URLs that are in the process of being deleted.
  std::set<GURL> urls_to_be_deleted_;

  // TODO(skym): Ideally the state is never a member field, but someone needs to
  // own it. Consider scoped_ptr and binding to callbacks?
  // State information about a query that's currently occurring, only valid
  // while a query is still outstanding.
  QueryHistoryState oustanding_query_state_;

  // Timer used to implement a timeout on a Web History response.
  base::OneShotTimer web_history_timer_;

  // HistoryService (local history) observer.
  ScopedObserver<HistoryService, HistoryServiceObserver>
      history_service_observer_;

  // WebHistoryService (synced history) observer.
  ScopedObserver<WebHistoryService, WebHistoryServiceObserver>
      web_history_service_observer_;

  // SyncService observer listens to late initialization of history sync.
  ScopedObserver<syncer::SyncService, syncer::SyncServiceObserver>
      sync_service_observer_;

  // Whether the last call to Web History returned synced results.
  bool has_synced_results_ = false;

  // Whether there are other forms of browsing history on the history server.
  bool has_other_forms_of_browsing_history_ = false;

  BrowsingHistoryDriver* driver_;

  HistoryService* local_history_;

  syncer::SyncService* sync_service_;

  // The clock used to vend times.
  std::unique_ptr<base::Clock> clock_;

  base::WeakPtrFactory<BrowsingHistoryService> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(BrowsingHistoryService);
};

}  // namespace history

#endif  // COMPONENTS_HISTORY_CORE_BROWSER_BROWSING_HISTORY_SERVICE_H_
