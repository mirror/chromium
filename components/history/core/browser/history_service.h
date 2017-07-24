// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_HISTORY_CORE_BROWSER_HISTORY_SERVICE_H_
#define COMPONENTS_HISTORY_CORE_BROWSER_HISTORY_SERVICE_H_

#include <stddef.h>
#include <stdint.h>

#include <memory>
#include <set>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/callback_list.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/sequenced_task_runner.h"
#include "base/strings/string16.h"
#include "base/task/cancelable_task_tracker.h"
#include "base/time/time.h"
#include "build/build_config.h"
#include "components/favicon_base/favicon_callback.h"
#include "components/favicon_base/favicon_usage_data.h"
#include "components/history/core/browser/history_types.h"
#include "components/history/core/browser/keyword_id.h"
#include "components/history/core/browser/typed_url_sync_bridge.h"
#include "components/history/core/browser/typed_url_syncable_service.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/sync/model/syncable_service.h"
#include "sql/init_status.h"
#include "ui/base/page_transition_types.h"

class GURL;
class HistoryQuickProviderTest;
class HistoryURLProvider;
class InMemoryURLIndexTest;
class SkBitmap;

namespace favicon {
class FaviconServiceImpl;
}  // namespace favicon

namespace sync_pb {
class HistoryDeleteDirectiveSpecifics;
}

namespace history {

struct DownloadRow;
struct HistoryAddPageArgs;
class HistoryBackend;
class HistoryDBTask;
struct HistoryDatabaseParams;
class HistoryServiceObserver;
class URLDatabase;
class WebHistoryService;

// The history service records page titles, and visit times, as well as
// (eventually) information about autocomplete.
//
// This service is thread safe. Each request callback is invoked in the
// thread that made the request.
class HistoryService : public syncer::SyncableService, public KeyedService {
 public:
  // Callback for value asynchronously returned by TopHosts().
  typedef base::Callback<void(const TopHostsList&)> TopHostsCallback;

  // Initializes the history service, returning true on success. On false, do
  // not call any other functions. The given directory will be used for storing
  // the history files.
  virtual bool Init(const HistoryDatabaseParams& history_database_params) = 0;

  // Returns true if the backend has finished loading.
  virtual bool BackendLoaded() = 0;

#if defined(OS_IOS)
  // Causes the history backend to commit any in-progress transactions. Called
  // when the application is being backgrounded.
  virtual void HandleBackgrounding() = 0;
#endif

  // Context ids are used to scope page IDs (see AddPage). These contexts
  // must tell us when they are being invalidated so that we can clear
  // out any cached data associated with that context.
  virtual void ClearCachedDataForContextID(ContextID context_id) = 0;

  // Triggers the backend to load if it hasn't already, and then returns the
  // in-memory URL database. The returned pointer may be null if the in-memory
  // database has not been loaded yet. This pointer is owned by the history
  // system. Callers should not store or cache this value.
  //
  // TODO(brettw) this should return the InMemoryHistoryBackend.
  virtual URLDatabase* InMemoryDatabase() = 0;

  // Following functions get URL information from in-memory database.
  // They return false if database is not available (e.g. not loaded yet) or the
  // URL does not exist.

  // Returns a pointer to the TypedURLSyncBridge owned by HistoryBackend.
  // This method should only be called from the history thread, because the
  // returned bridge is intended to be accessed only via the history thread.
  virtual TypedURLSyncBridge* GetTypedURLSyncBridge() const = 0;

  // Returns a pointer to the TypedUrlSyncableService owned by HistoryBackend.
  // This method should only be called from the history thread, because the
  // returned service is intended to be accessed only via the history thread.
  virtual TypedUrlSyncableService* GetTypedUrlSyncableService() const = 0;

  // Callback for value asynchronously returned by
  // GetCountsAndLastVisitForOrigins().
  typedef base::Callback<void(const OriginCountAndLastVisitMap&)>
      GetCountsAndLastVisitForOriginsCallback;

  // Computes the |num_hosts| most-visited hostnames in the past 30 days and
  // returns a list of those hosts paired with their visit counts. The following
  // caveats apply:
  // 1. Hostnames are stripped of their 'www.' prefix. Visits to foo.com and
  //    www.foo.com are summed into the resultant foo.com entry.
  // 2. Ports and schemes are ignored. Visits to http://foo.com/ and
  //    https://foo.com:567/ are summed into the resultant foo.com entry.
  // 3. If the history is abnormally large and diverse, the function will give
  //    up early and return an approximate list.
  // 4. Only http://, https://, and ftp:// URLs are counted.
  //
  // Note: Virtual needed for mocking.
  virtual void TopHosts(size_t num_hosts,
                        const TopHostsCallback& callback) const = 0;

  // Gets the counts and most recent visit date of URLs that belong to |origins|
  // in the history database.
  virtual void GetCountsAndLastVisitForOrigins(
      const std::set<GURL>& origins,
      const GetCountsAndLastVisitForOriginsCallback& callback) const = 0;

  // Returns, for the given URL, a 0-based index into the list produced by
  // TopHosts(), corresponding to that URL's host. If TopHosts() has not
  // previously been run, or the host is not in the top kMaxTopHosts, returns
  // kMaxTopHosts.
  //
  // Note: Virtual needed for mocking.
  virtual void HostRankIfAvailable(
      const GURL& url,
      const base::Callback<void(int)>& callback) const = 0;

  // Navigation ----------------------------------------------------------------

  // Adds the given canonical URL to history with the given time as the visit
  // time. Referrer may be the empty string.
  //
  // The supplied context id is used to scope the given page ID. Page IDs
  // are only unique inside a given context, so we need that to differentiate
  // them.
  //
  // The context/page ids can be null if there is no meaningful tracking
  // information that can be performed on the given URL. The 'nav_entry_id'
  // should be the unique ID of the current navigation entry in the given
  // process.
  //
  // 'redirects' is an array of redirect URLs leading to this page, with the
  // page itself as the last item (so when there is no redirect, it will have
  // one entry). If there are no redirects, this array may also be empty for
  // the convenience of callers.
  //
  // 'did_replace_entry' is true when the navigation entry for this page has
  // replaced the existing entry. A non-user initiated redirect causes such
  // replacement.
  //
  // All "Add Page" functions will update the visited link database.
  virtual void AddPage(const GURL& url,
                       base::Time time,
                       ContextID context_id,
                       int nav_entry_id,
                       const GURL& referrer,
                       const RedirectList& redirects,
                       ui::PageTransition transition,
                       VisitSource visit_source,
                       bool did_replace_entry) = 0;

  // For adding pages to history where no tracking information can be done.
  virtual void AddPage(const GURL& url,
                       base::Time time,
                       VisitSource visit_source) = 0;

  // All AddPage variants end up here.
  virtual void AddPage(const HistoryAddPageArgs& add_page_args) = 0;

  // Adds an entry for the specified url without creating a visit. This should
  // only be used when bookmarking a page, otherwise the row leaks in the
  // history db (it never gets cleaned).
  virtual void AddPageNoVisitForBookmark(const GURL& url,
                                         const base::string16& title) = 0;

  // Sets the title for the given page. The page should be in history. If it
  // is not, this operation is ignored.
  virtual void SetPageTitle(const GURL& url, const base::string16& title) = 0;

  // Updates the history database with a page's ending time stamp information.
  // The page can be identified by the combination of the context id, the
  // navigation entry id and the url.
  virtual void UpdateWithPageEndTime(ContextID context_id,
                                     int nav_entry_id,
                                     const GURL& url,
                                     base::Time end_ts) = 0;

  // Querying ------------------------------------------------------------------

  // Returns the information about the requested URL. If the URL is found,
  // success will be true and the information will be in the URLRow parameter.
  // On success, the visits, if requested, will be sorted by date. If they have
  // not been requested, the pointer will be valid, but the vector will be
  // empty.
  //
  // If success is false, neither the row nor the vector will be valid.
  typedef base::OnceCallback<void(
      bool,  // Success flag, when false, nothing else is valid.
      const URLRow&,
      const VisitVector&)>
      QueryURLCallback;

  // Queries the basic information about the URL in the history database. If
  // the caller is interested in the visits (each time the URL is visited),
  // set |want_visits| to true. If these are not needed, the function will be
  // faster by setting this to false.
  virtual base::CancelableTaskTracker::TaskId QueryURL(
      const GURL& url,
      bool want_visits,
      QueryURLCallback callback,
      base::CancelableTaskTracker* tracker) = 0;

  // Provides the result of a query. See QueryResults in history_types.h.
  // The common use will be to use QueryResults.Swap to suck the contents of
  // the results out of the passed in parameter and take ownership of them.
  typedef base::Callback<void(QueryResults*)> QueryHistoryCallback;

  // Queries all history with the given options (see QueryOptions in
  // history_types.h).  If empty, all results matching the given options
  // will be returned.
  virtual base::CancelableTaskTracker::TaskId QueryHistory(
      const base::string16& text_query,
      const QueryOptions& options,
      const QueryHistoryCallback& callback,
      base::CancelableTaskTracker* tracker) = 0;

  // Called when the results of QueryRedirectsFrom are available.
  // The given vector will contain a list of all redirects, not counting
  // the original page. If A redirects to B which redirects to C, the vector
  // will contain [B, C], and A will be in 'from_url'.
  //
  // For QueryRedirectsTo, the order is reversed. For A->B->C, the vector will
  // contain [B, A] and C will be in 'to_url'.
  //
  // If there is no such URL in the database or the most recent visit has no
  // redirect, the vector will be empty. If the given page has redirected to
  // multiple destinations, this will pick a random one.
  typedef base::Callback<void(const RedirectList*)> QueryRedirectsCallback;

  // Schedules a query for the most recent redirect coming out of the given
  // URL. See the RedirectQuerySource above, which is guaranteed to be called
  // if the request is not canceled.
  virtual base::CancelableTaskTracker::TaskId QueryRedirectsFrom(
      const GURL& from_url,
      const QueryRedirectsCallback& callback,
      base::CancelableTaskTracker* tracker) = 0;

  // Schedules a query to get the most recent redirects ending at the given
  // URL.
  virtual base::CancelableTaskTracker::TaskId QueryRedirectsTo(
      const GURL& to_url,
      const QueryRedirectsCallback& callback,
      base::CancelableTaskTracker* tracker) = 0;

  // Requests the number of user-visible visits (i.e. no redirects or subframes)
  // to all urls on the same scheme/host/port as |url|.  This is only valid for
  // HTTP and HTTPS URLs.
  typedef base::Callback<void(
      bool,         // Were we able to determine the # of visits?
      int,          // Number of visits.
      base::Time)>  // Time of first visit. Only set if bool
                    // is true and int is > 0.
      GetVisibleVisitCountToHostCallback;

  virtual base::CancelableTaskTracker::TaskId GetVisibleVisitCountToHost(
      const GURL& url,
      const GetVisibleVisitCountToHostCallback& callback,
      base::CancelableTaskTracker* tracker) = 0;

  // Request the |result_count| most visited URLs and the chain of
  // redirects leading to each of these URLs. |days_back| is the
  // number of days of history to use. Used by TopSites.
  typedef base::Callback<void(const MostVisitedURLList*)>
      QueryMostVisitedURLsCallback;

  virtual base::CancelableTaskTracker::TaskId QueryMostVisitedURLs(
      int result_count,
      int days_back,
      const QueryMostVisitedURLsCallback& callback,
      base::CancelableTaskTracker* tracker) = 0;

  // Statistics ----------------------------------------------------------------

  // Gets the number of URLs as seen in chrome://history within the time range
  // [|begin_time|, |end_time|). Each URL is counted only once per day. For
  // determination of the date, timestamps are converted to dates using local
  // time.
  typedef base::Callback<void(HistoryCountResult)> GetHistoryCountCallback;

  virtual base::CancelableTaskTracker::TaskId GetHistoryCount(
      const base::Time& begin_time,
      const base::Time& end_time,
      const GetHistoryCountCallback& callback,
      base::CancelableTaskTracker* tracker) = 0;

  // Database management operations --------------------------------------------

  // Delete all the information related to a single url.
  virtual void DeleteURL(const GURL& url) = 0;

  // Delete all the information related to a list of urls.  (Deleting
  // URLs one by one is slow as it has to flush to disk each time.)
  virtual void DeleteURLsForTest(const std::vector<GURL>& urls) = 0;

  // Removes all visits in the selected time range (including the
  // start time), updating the URLs accordingly. This deletes any
  // associated data. This function also deletes the associated
  // favicons, if they are no longer referenced. |callback| runs when
  // the expiration is complete. You may use null Time values to do an
  // unbounded delete in either direction.
  // If |restrict_urls| is not empty, only visits to the URLs in this set are
  // removed.
  virtual void ExpireHistoryBetween(const std::set<GURL>& restrict_urls,
                                    base::Time begin_time,
                                    base::Time end_time,
                                    const base::Closure& callback,
                                    base::CancelableTaskTracker* tracker) = 0;

  // Removes all visits to specified URLs in specific time ranges.
  // This is the equivalent ExpireHistoryBetween() once for each element in the
  // vector. The fields of |ExpireHistoryArgs| map directly to the arguments of
  // of ExpireHistoryBetween().
  virtual void ExpireHistory(const std::vector<ExpireHistoryArgs>& expire_list,
                             const base::Closure& callback,
                             base::CancelableTaskTracker* tracker) = 0;

  // Removes all visits to the given URLs in the specified time range. Calls
  // ExpireHistoryBetween() to delete local visits, and handles deletion of
  // synced visits if appropriate.
  virtual void ExpireLocalAndRemoteHistoryBetween(
      WebHistoryService* web_history,
      const std::set<GURL>& restrict_urls,
      base::Time begin_time,
      base::Time end_time,
      const base::Closure& callback,
      base::CancelableTaskTracker* tracker) = 0;

  // Processes the given |delete_directive| and sends it to the
  // SyncChangeProcessor (if it exists).  Returns any error resulting
  // from sending the delete directive to sync.
  virtual syncer::SyncError ProcessLocalDeleteDirective(
      const sync_pb::HistoryDeleteDirectiveSpecifics& delete_directive) = 0;

  // Downloads -----------------------------------------------------------------

  // Implemented by the caller of 'CreateDownload' below, and is called when the
  // history service has created a new entry for a download in the history db.
  typedef base::Callback<void(bool)> DownloadCreateCallback;

  // Begins a history request to create a new row for a download. 'info'
  // contains all the download's creation state, and 'callback' runs when the
  // history service request is complete. The callback is called on the thread
  // that calls CreateDownload().
  virtual void CreateDownload(const DownloadRow& info,
                              const DownloadCreateCallback& callback) = 0;

  // Implemented by the caller of 'GetNextDownloadId' below, and is called with
  // the maximum id of all downloads records in the database plus 1.
  typedef base::Callback<void(uint32_t)> DownloadIdCallback;

  // Responds on the calling thread with the maximum id of all downloads records
  // in the database plus 1.
  virtual void GetNextDownloadId(const DownloadIdCallback& callback) = 0;

  // Implemented by the caller of 'QueryDownloads' below, and is called when the
  // history service has retrieved a list of all download state. The call
  typedef base::Callback<void(std::unique_ptr<std::vector<DownloadRow>>)>
      DownloadQueryCallback;

  // Begins a history request to retrieve the state of all downloads in the
  // history db. 'callback' runs when the history service request is complete,
  // at which point 'info' contains an array of DownloadRow, one per
  // download. The callback is called on the thread that calls QueryDownloads().
  virtual void QueryDownloads(const DownloadQueryCallback& callback) = 0;

  // Called to update the history service about the current state of a download.
  // This is a 'fire and forget' query, so just pass the relevant state info to
  // the database with no need for a callback.
  virtual void UpdateDownload(const DownloadRow& data,
                              bool should_commit_immediately) = 0;

  // Permanently remove some downloads from the history system. This is a 'fire
  // and forget' operation.
  virtual void RemoveDownloads(const std::set<uint32_t>& ids) = 0;

  // Keyword search terms -----------------------------------------------------

  // Sets the search terms for the specified url and keyword. url_id gives the
  // id of the url, keyword_id the id of the keyword and term the search term.
  virtual void SetKeywordSearchTermsForURL(const GURL& url,
                                           KeywordID keyword_id,
                                           const base::string16& term) = 0;

  // Deletes all search terms for the specified keyword.
  virtual void DeleteAllSearchTermsForKeyword(KeywordID keyword_id) = 0;

  // Deletes any search term corresponding to |url|.
  virtual void DeleteKeywordSearchTermForURL(const GURL& url) = 0;

  // Deletes all URL and search term entries matching the given |term| and
  // |keyword_id|.
  virtual void DeleteMatchingURLsForKeyword(KeywordID keyword_id,
                                            const base::string16& term) = 0;

  // Bookmarks -----------------------------------------------------------------

  // Notification that a URL is no longer bookmarked.
  virtual void URLsNoLongerBookmarked(const std::set<GURL>& urls) = 0;

  // Observers -----------------------------------------------------------------

  // Adds/Removes an Observer.
  virtual void AddObserver(HistoryServiceObserver* observer) = 0;
  virtual void RemoveObserver(HistoryServiceObserver* observer) = 0;

  // Generic Stuff -------------------------------------------------------------

  // Schedules a HistoryDBTask for running on the history backend thread. See
  // HistoryDBTask for details on what this does. Takes ownership of |task|.
  virtual base::CancelableTaskTracker::TaskId ScheduleDBTask(
      std::unique_ptr<HistoryDBTask> task,
      base::CancelableTaskTracker* tracker) = 0;

  // Callback for when favicon data changes. Contains a std::set of page URLs
  // (e.g. http://www.google.com) for which the favicon data has changed and the
  // icon URL (e.g. http://www.google.com/favicon.ico) for which the favicon
  // data has changed. It is valid to call the callback with non-empty
  // "page URLs" and no "icon URL" and vice versa.
  typedef base::Callback<void(const std::set<GURL>&, const GURL&)>
      OnFaviconsChangedCallback;

  // Add a callback to the list. The callback will remain registered until the
  // returned Subscription is destroyed. The Subscription must be destroyed
  // before HistoryService is destroyed.
  virtual std::unique_ptr<base::CallbackList<void(const std::set<GURL>&,
                                                  const GURL&)>::Subscription>
  AddFaviconsChangedCallback(const OnFaviconsChangedCallback& callback)
      WARN_UNUSED_RESULT = 0;

  // Testing -------------------------------------------------------------------

  // Runs |flushed| after bouncing off the history thread.
  virtual void FlushForTest(const base::Closure& flushed) = 0;

  // Designed for unit tests, this passes the given task on to the history
  // backend to be called once the history backend has terminated. This allows
  // callers to know when the history thread is complete and the database files
  // can be deleted and the next test run. Otherwise, the history thread may
  // still be running, causing problems in subsequent tests.
  //
  // There can be only one closing task, so this will override any previously
  // set task. We will take ownership of the pointer and delete it when done.
  // The task will be run on the calling thread (this function is threadsafe).
  virtual void SetOnBackendDestroyTask(const base::Closure& task) = 0;

  // Used for unit testing and potentially importing to get known information
  // into the database. This assumes the URL doesn't exist in the database
  //
  // Calling this function many times may be slow because each call will
  // dispatch to the history thread and will be a separate database
  // transaction. If this functionality is needed for importing many URLs,
  // callers should use AddPagesWithDetails() instead.
  //
  // Note that this routine (and AddPageWithDetails()) always adds a single
  // visit using the |last_visit| timestamp, and a PageTransition type of LINK,
  // if |visit_source| != SYNCED.
  virtual void AddPageWithDetails(const GURL& url,
                                  const base::string16& title,
                                  int visit_count,
                                  int typed_count,
                                  base::Time last_visit,
                                  bool hidden,
                                  VisitSource visit_source) = 0;

  // The same as AddPageWithDetails() but takes a vector.
  virtual void AddPagesWithDetails(const URLRows& info,
                                   VisitSource visit_source) = 0;

  virtual base::WeakPtr<HistoryService> AsWeakPtr() = 0;

 private:
  friend class favicon::FaviconServiceImpl;
  friend class HistoryBackend;
  friend class ::HistoryQuickProviderTest;
  friend class ::HistoryURLProvider;
  friend class HQPPerfTestOnePopularURL;
  friend class ::InMemoryURLIndexTest;

  // Low-level Init().  Same as the public version, but adds a |no_db| parameter
  // that is only set by unittests which causes the backend to not init its DB.
  virtual bool Init(bool no_db,
                    const HistoryDatabaseParams& history_database_params) = 0;

  // Called by the HistoryURLProvider class to schedule an autocomplete, it
  // will be called back on the internal history thread with the history
  // database so it can query. See history_url_provider.h for a diagram.
  virtual void ScheduleAutocomplete(
      const base::Callback<void(HistoryBackend*, URLDatabase*)>& callback) = 0;

  // Returns the backend object, which may or may not exist yet.
  virtual HistoryBackend* GetHistoryBackend() = 0;

  // Favicon -------------------------------------------------------------------

  // These favicon methods are exposed to the FaviconService. Instead of calling
  // these methods directly you should call the respective method on the
  // FaviconService.

  // Used by FaviconService to get the favicon bitmaps from the history backend
  // whose edge sizes most closely match |desired_sizes| for |icon_type|. If
  // |desired_sizes| has a '0' entry, the largest favicon bitmap for
  // |icon_type| is returned. The returned FaviconBitmapResults will have at
  // most one result for each entry in |desired_sizes|. If a favicon bitmap is
  // determined to be the best candidate for multiple |desired_sizes| there will
  // be fewer results.
  virtual base::CancelableTaskTracker::TaskId GetFavicon(
      const GURL& icon_url,
      favicon_base::IconType icon_type,
      const std::vector<int>& desired_sizes,
      const favicon_base::FaviconResultsCallback& callback,
      base::CancelableTaskTracker* tracker) = 0;

  // Used by the FaviconService to get favicons mapped to |page_url| for
  // |icon_types| whose edge sizes most closely match |desired_sizes|. If
  // |desired_sizes| has a '0' entry, the largest favicon bitmap for
  // |icon_types| is returned. The returned FaviconBitmapResults will have at
  // most one result for each entry in |desired_sizes|. If a favicon bitmap is
  // determined to be the best candidate for multiple |desired_sizes| there
  // will be fewer results.
  virtual base::CancelableTaskTracker::TaskId GetFaviconsForURL(
      const GURL& page_url,
      int icon_types,
      const std::vector<int>& desired_sizes,
      const favicon_base::FaviconResultsCallback& callback,
      base::CancelableTaskTracker* tracker) = 0;

  // Used by FaviconService to find the first favicon bitmap whose width and
  // height are greater than that of |minimum_size_in_pixels|. This searches
  // for icons by IconType. Each element of |icon_types| is a bitmask of
  // IconTypes indicating the types to search for.
  // If the largest icon of |icon_types[0]| is not larger than
  // |minimum_size_in_pixel|, the next icon types of
  // |icon_types| will be searched and so on.
  // If no icon is larger than |minimum_size_in_pixel|, the largest one of all
  // icon types in |icon_types| is returned.
  // This feature is especially useful when some types of icon is preferred as
  // long as its size is larger than a specific value.
  virtual base::CancelableTaskTracker::TaskId GetLargestFaviconForURL(
      const GURL& page_url,
      const std::vector<int>& icon_types,
      int minimum_size_in_pixels,
      const favicon_base::FaviconRawBitmapCallback& callback,
      base::CancelableTaskTracker* tracker) = 0;

  // Used by the FaviconService to get the favicon bitmap which most closely
  // matches |desired_size| from the favicon with |favicon_id| from the history
  // backend. If |desired_size| is 0, the largest favicon bitmap for
  // |favicon_id| is returned.
  virtual base::CancelableTaskTracker::TaskId GetFaviconForID(
      favicon_base::FaviconID favicon_id,
      int desired_size,
      const favicon_base::FaviconResultsCallback& callback,
      base::CancelableTaskTracker* tracker) = 0;

  // Maps |page_url| to the favicon at |icon_url| if there is an entry in the
  // database for |icon_url| and |icon_type|. This occurs when there is a
  // mapping from a different page URL to |icon_url|. The favicon bitmaps whose
  // edge sizes most closely match |desired_sizes| from the favicons which were
  // just mapped to |page_url| are returned. If |desired_sizes| has a '0' entry,
  // the largest favicon bitmap is returned.
  virtual base::CancelableTaskTracker::TaskId UpdateFaviconMappingsAndFetch(
      const GURL& page_url,
      const GURL& icon_url,
      favicon_base::IconType icon_type,
      const std::vector<int>& desired_sizes,
      const favicon_base::FaviconResultsCallback& callback,
      base::CancelableTaskTracker* tracker) = 0;

  // Used by FaviconService to set a favicon for |page_url| and |icon_url| with
  // |pixel_size|.
  // Example:
  //   |page_url|: www.google.com
  // 2 favicons in history for |page_url|:
  //   www.google.com/a.ico  16x16
  //   www.google.com/b.ico  32x32
  // MergeFavicon(|page_url|, www.google.com/a.ico, ..., ..., 16x16)
  //
  // Merging occurs in the following manner:
  // 1) |page_url| is set to map to only to |icon_url|. In order to not lose
  //    data, favicon bitmaps mapped to |page_url| but not to |icon_url| are
  //    copied to the favicon at |icon_url|.
  //    For the example above, |page_url| will only be mapped to a.ico.
  //    The 32x32 favicon bitmap at b.ico is copied to a.ico
  // 2) |bitmap_data| is added to the favicon at |icon_url|, overwriting any
  //    favicon bitmaps of |pixel_size|.
  //    For the example above, |bitmap_data| overwrites the 16x16 favicon
  //    bitmap for a.ico.
  // TODO(pkotwicz): Remove once no longer required by sync.
  virtual void MergeFavicon(const GURL& page_url,
                            const GURL& icon_url,
                            favicon_base::IconType icon_type,
                            scoped_refptr<base::RefCountedMemory> bitmap_data,
                            const gfx::Size& pixel_size) = 0;

  // Used by the FaviconService to replace all of the favicon bitmaps mapped to
  // |page_url| for |icon_type|.
  // Use MergeFavicon() if |bitmaps| is incomplete, and favicon bitmaps in the
  // database should be preserved if possible. For instance, favicon bitmaps
  // from sync are 1x only. MergeFavicon() is used to avoid deleting the 2x
  // favicon bitmap if it is present in the history backend.
  virtual void SetFavicons(const GURL& page_url,
                           favicon_base::IconType icon_type,
                           const GURL& icon_url,
                           const std::vector<SkBitmap>& bitmaps) = 0;

  // Same as SetFavicons with three differences:
  // 1) It will be a no-op if there is an existing cached favicon for *any* type
  //    for |page_url|.
  // 2) If |icon_url| is known to the database, |bitmaps| will be ignored (i.e.
  //    the icon won't be overwritten) but the mappings from |page_url| to
  //    |icon_url| will be stored (conditioned to point 1 above).
  // 3) If |icon_url| is stored, it will be marked as "on-demand".
  //
  // On-demand favicons are those that are fetched without visiting their page.
  // For this reason, their life-time cannot be bound to the life-time of the
  // corresponding visit in history.
  // - These bitmaps are evicted from the database based on the last time they
  //   get requested. The last requested time is initially set to Now() and is
  //   further updated by calling TouchOnDemandFavicon().
  // - Furthermore, on-demand bitmaps are immediately marked as expired. Hence,
  //   they are always replaced by standard favicons whenever their page gets
  //   visited.
  // The callback will receive whether the write actually happened.
  virtual void SetOnDemandFavicons(const GURL& page_url,
                                   favicon_base::IconType icon_type,
                                   const GURL& icon_url,
                                   const std::vector<SkBitmap>& bitmaps,
                                   base::Callback<void(bool)> callback) = 0;

  // Used by the FaviconService to mark the favicon for the page as being out
  // of date.
  virtual void SetFaviconsOutOfDateForPage(const GURL& page_url) = 0;

  // Mark that the on-demand favicon at |icon_url| was requested now. This
  // postpones the automatic eviction of the favicon from the database. Not all
  // calls end up in a write into the DB:
  // - it is no-op if the bitmaps are not stored using SetOnDemandFavicons();
  // - the updates of the "last requested time" have limited frequency for each
  //   particular favicon (e.g. once per week). This limits the overhead of
  //   cache management for on-demand favicons.
  virtual void TouchOnDemandFavicon(const GURL& icon_url) = 0;

  // Used by the FaviconService for importing many favicons for many pages at
  // once. The pages must exist, any favicon sets for unknown pages will be
  // discarded. Existing favicons will not be overwritten.
  virtual void SetImportedFavicons(
      const favicon_base::FaviconUsageDataList& favicon_usage) = 0;
};

}  // namespace history

#endif  // COMPONENTS_HISTORY_CORE_BROWSER_HISTORY_SERVICE_H_
