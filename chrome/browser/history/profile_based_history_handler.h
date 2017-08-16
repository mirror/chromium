// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_HISTORY_PROFILE_BASED_HISTORY_HANDLER_H_
#define CHROME_BROWSER_HISTORY_PROFILE_BASED_HISTORY_HANDLER_H_

#include <vector>

#include "components/history/core/browser/browsing_history_service_handler.h"

namespace history {
class WebHistoryService;
}

class GURL;
class Profile;

// Base class that implements non-interface half of
// BrowsingHistoryServiceHandler, backed by a profile.
class ProfileBasedHistoryHandler : public BrowsingHistoryServiceHandler {
 public:
  // BrowsingHistoryServiceHandler implementation.
  void OnRemoveVisits(
      const std::vector<history::ExpireHistoryArgs>& expire_list) override;
  bool AllowHistoryDeletions() override;
  bool ShouldHideWebHistoryUrl(const GURL& url) override;
  history::WebHistoryService* GetWebHistoryService() override;
  void ShouldShowNoticeAboutOtherFormsOfBrowsingHistory(
      const syncer::SyncService* sync_service,
      history::WebHistoryService* history_service,
      base::Callback<void(bool)> callback) override;

  virtual Profile* GetProfile() = 0;

 protected:
  ProfileBasedHistoryHandler() {}

 private:
  DISALLOW_COPY_AND_ASSIGN(ProfileBasedHistoryHandler);
};

#endif  // CHROME_BROWSER_HISTORY_PROFILE_BASED_HISTORY_HANDLER_H_
