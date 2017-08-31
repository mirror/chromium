// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_HISTORY_HISTORY_TAB_HELPER_H_
#define IOS_CHROME_BROWSER_HISTORY_HISTORY_TAB_HELPER_H_

#include "base/time/time.h"
#include "components/history/core/browser/history_types.h"
#include "ios/web/public/web_state/web_state_observer.h"
#include "ios/web/public/web_state/web_state_user_data.h"

class GURL;

namespace history {
class HistoryService;
}  // namespace history

namespace web {
class NavigationItem;
}  // namespace web

class HistoryTabHelper : public web::WebStateObserver,
                         public web::WebStateUserData<HistoryTabHelper> {
 public:
  ~HistoryTabHelper() override;

  // Updates history with the specified navigation.
  void UpdateHistoryForNavigation(
      const history::HistoryAddPageArgs& add_page_args);

  // Sends the page title to the history service.
  void UpdateHistoryPageTitle(const web::NavigationItem& item);

  // Returns the history::HistoryAddPageArgs to use for adding a page to
  // history.
  history::HistoryAddPageArgs CreateHistoryAddPageArgs(const GURL& virtual_url,
                                                       base::Time timestamp,
                                                       int nav_entry_id);

 private:
  friend class web::WebStateUserData<HistoryTabHelper>;

  explicit HistoryTabHelper(web::WebState* web_state);

  // web::WebStateObserver implementation.

  // Helper function to return the history service. May return NULL, in which
  // case no history entries should be added.
  history::HistoryService* GetHistoryService();
};

#endif  // IOS_CHROME_BROWSER_HISTORY_HISTORY_TAB_HELPER_H_
