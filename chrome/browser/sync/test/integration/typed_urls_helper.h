// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SYNC_TEST_INTEGRATION_TYPED_URLS_HELPER_H_
#define CHROME_BROWSER_SYNC_TEST_INTEGRATION_TYPED_URLS_HELPER_H_

#include <string>
#include <vector>

#include "base/single_thread_task_runner.h"
#include "chrome/browser/sync/test/integration/multi_client_status_change_checker.h"
#include "components/history/core/browser/history_types.h"
#include "components/history/core/browser/typed_url_sync_bridge.h"
#include "ui/base/page_transition_types.h"

namespace base {
class Time;
}

namespace typed_urls_helper {

// Gets the typed URLs from a specific sync profile.
history::URLRows GetTypedUrlsFromClient(int index);

// Gets a specific url from a specific sync profile. Returns false if the URL
// was not found in the history DB.
bool GetUrlFromClient(int index, const GURL& url, history::URLRow* row);

// Gets the visits for a URL from a specific sync profile.
history::VisitVector GetVisitsFromClient(int index, history::URLID id);

// Gets the visits for a URL from a specific sync profile. Like above, but
// takes a GURL instead of URLID. Returns empty vector if |url| is not returned
// by GetUrlFromClient().
history::VisitVector GetVisitsForURLFromClient(int index, const GURL& url);

// Removes the passed |visits| from a specific sync profile.
void RemoveVisitsFromClient(int index, const history::VisitVector& visits);

// Adds a URL to the history DB for a specific sync profile (just registers a
// new visit if the URL already exists) using a TYPED PageTransition.
void AddUrlToHistory(int index, const GURL& url);

// Adds a URL to the history DB for a specific sync profile (just registers a
// new visit if the URL already exists), using the passed PageTransition.
void AddUrlToHistoryWithTransition(int index,
                                   const GURL& url,
                                   ui::PageTransition transition,
                                   history::VisitSource source);

// Adds a URL to the history DB for a specific sync profile (just registers a
// new visit if the URL already exists), using the passed PageTransition and
// timestamp.
void AddUrlToHistoryWithTimestamp(int index,
                                  const GURL& url,
                                  ui::PageTransition transition,
                                  history::VisitSource source,
                                  const base::Time& timestamp);

// Deletes a URL from the history DB for a specific sync profile.
void DeleteUrlFromHistory(int index, const GURL& url);

// Deletes a list of URLs from the history DB for a specific sync
// profile.
void DeleteUrlsFromHistory(int index, const std::vector<GURL>& urls);

// Modifies an URL stored in history by setting a new title.
void SetPageTitle(int index, const GURL& url, const std::string& title);

// Returns true if all clients have the same URLs.
bool CheckAllProfilesHaveSameURLs();

// Return ture if a specific sync metadata has the typed url.
bool CheckSyncHasURLMetadata(int index, const GURL& url);

// Checks that the two vectors contain the same set of URLRows (possibly in
// a different order).
bool CheckURLRowVectorsAreEqual(const history::URLRows& left,
                                const history::URLRows& right);

// Checks that the passed URLRows are equivalent.
bool CheckURLRowsAreEqual(const history::URLRow& left,
                          const history::URLRow& right);

// Returns true if two sets of visits are equivalent.
bool AreVisitsEqual(const history::VisitVector& visit1,
                    const history::VisitVector& visit2);

// Returns true if there are no duplicate visit times.
bool AreVisitsUnique(const history::VisitVector& visits);

// Returns a unique timestamp to use when generating page visits
// (HistoryService does not like having identical timestamps and will modify
// the timestamps behind the scenes if it encounters them, which leads to
// spurious test failures when the resulting timestamps aren't what we
// expect).
base::Time GetTimestamp();

history::HistoryBackend* GetHistoryBackend(
    const history::TypedURLSyncBridge* bridge);

}  // namespace typed_urls_helper

// Checker that blocks until all clients have the same URLs.
class ProfilesHaveSameURLsChecker : public MultiClientStatusChangeChecker {
 public:
  ProfilesHaveSameURLsChecker();

  // Implementation of StatusChangeChecker.
  bool IsExitConditionSatisfied() override;
  std::string GetDebugMessage() const override;
};

class TypedURLChecker : public StatusChangeChecker,
                        public history::HistoryBackendObserver {
 public:
  TypedURLChecker(int index, const std::string& url);
  ~TypedURLChecker() override;

  bool IsExitConditionSatisfied() override;

  std::string GetDebugMessage() const override;

  // history::HistoryBackendObserver:
  void OnURLVisited(history::HistoryBackend* history_backend,
                    ui::PageTransition transition,
                    const history::URLRow& row,
                    const history::RedirectList& redirects,
                    base::Time visit_time) override;
  void OnURLsModified(history::HistoryBackend* history_backend,
                      const history::URLRows& changed_urls) override;
  void OnURLsDeleted(history::HistoryBackend* history_backend,
                     bool all_history,
                     bool expired,
                     const history::URLRows& deleted_rows,
                     const std::set<GURL>& favicon_urls) override;

 private:
  int index_;
  const std::string url_;

  // Tracks observed history backend, for receiving updates from history
  // backend.
  ScopedObserver<history::HistoryBackend, HistoryBackendObserver>
      history_backend_observer_;

  scoped_refptr<base::SingleThreadTaskRunner> origin_task_runner_;
};

#endif  // CHROME_BROWSER_SYNC_TEST_INTEGRATION_TYPED_URLS_HELPER_H_
