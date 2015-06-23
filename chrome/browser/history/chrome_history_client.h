// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_HISTORY_CHROME_HISTORY_CLIENT_H_
#define CHROME_BROWSER_HISTORY_CHROME_HISTORY_CLIENT_H_

#include "base/macros.h"
#include "components/history/core/browser/history_client.h"

namespace bookmarks {
class BookmarkModel;
}

// This class implements history::HistoryClient to abstract operations that
// depend on Chrome environment.
class ChromeHistoryClient : public history::HistoryClient {
 public:
  explicit ChromeHistoryClient(bookmarks::BookmarkModel* bookmark_model);
  ~ChromeHistoryClient() override;

  // history::HistoryClient implementation.
  void Shutdown() override;
  bool CanAddURL(const GURL& url) override;
  void NotifyProfileError(sql::InitStatus init_status) override;
  scoped_ptr<history::HistoryBackendClient> CreateBackendClient() override;

 private:
  // BookmarkModel instance providing access to bookmarks. May be null during
  // testing but must outlive ChromeHistoryClient if non-null.
  bookmarks::BookmarkModel* bookmark_model_;

  DISALLOW_COPY_AND_ASSIGN(ChromeHistoryClient);
};

#endif  // CHROME_BROWSER_HISTORY_CHROME_HISTORY_CLIENT_H_
