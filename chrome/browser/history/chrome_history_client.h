// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_HISTORY_CHROME_HISTORY_CLIENT_H_
#define CHROME_BROWSER_HISTORY_CHROME_HISTORY_CLIENT_H_

#include "base/macros.h"
#include "components/history/core/browser/history_client.h"
#include "components/history/core/browser/history_service_observer.h"
#include "components/history/core/browser/top_sites_observer.h"

class BookmarkModel;
class HistoryService;
class Profile;

namespace history {
class TopSites;
}

// This class implements history::HistoryClient to abstract operations that
// depend on Chrome environment.
class ChromeHistoryClient : public history::HistoryClient,
                            public history::HistoryServiceObserver,
                            public history::TopSitesObserver {
 public:
  explicit ChromeHistoryClient(BookmarkModel* bookmark_model,
                               Profile* profile,
                               history::TopSites* top_sites);
  virtual ~ChromeHistoryClient();

  // TODO(sdefresne): once NOTIFICATION_HISTORY_URL* notifications are no
  // longer used, remove this reference to the HistoryService from the
  // ChromeHistoryClient, http://crbug.com/42178
  void SetHistoryService(HistoryService* history_service);

  // history::HistoryClient:
  virtual void BlockUntilBookmarksLoaded() override;
  virtual bool IsBookmarked(const GURL& url) override;
  virtual void GetBookmarks(
      std::vector<history::URLAndTitle>* bookmarks) override;
  virtual void NotifyProfileError(sql::InitStatus init_status) override;
  virtual bool ShouldReportDatabaseError() override;

  // KeyedService:
  virtual void Shutdown() override;

  // HistoryServiceObserver:
  virtual void OnURLVisited(HistoryService* history_service,
                            ui::PageTransition transition,
                            const history::URLRow& row,
                            const history::RedirectList& redirects,
                            base::Time visit_time) override;

  // TopSitesObserver:
  virtual void TopSitesLoaded(history::TopSites* top_sites) override;
  virtual void TopSitesChanged(history::TopSites* top_sites) override;

 private:
  // The BookmarkModel, this should outlive ChromeHistoryClient.
  BookmarkModel* bookmark_model_;
  Profile* profile_;
  HistoryService* history_service_;
  // The TopSites object is owned by the Profile (see
  // chrome/browser/profiles/profile_impl.h)
  // and lazily constructed by the  getter.
  // ChromeHistoryClient is a KeyedService linked to the Profile lifetime by the
  // ChromeHistoryClientFactory (which is a BrowserContextKeyedServiceFactory).
  // Before the Profile is destroyed, all the KeyedService Shutdown methods are
  // called, and the Profile is fully constructed before any of the KeyedService
  // can  be constructed. The TopSites does not use the HistoryService nor the
  // HistoryClient during construction (it uses it later, but supports getting
  // an NULL  pointer).
  history::TopSites* top_sites_;

  DISALLOW_COPY_AND_ASSIGN(ChromeHistoryClient);
};

#endif  // CHROME_BROWSER_HISTORY_CHROME_HISTORY_CLIENT_H_
