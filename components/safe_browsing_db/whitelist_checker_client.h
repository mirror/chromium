// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
#ifndef COMPONENTS_SAFE_BROWSING_DB_WHITELIST_CHECKER_CLIENT_H_
#define COMPONENTS_SAFE_BROWSING_DB_WHITELIST_CHECKER_CLIENT_H_

#include "base/memory/ptr_util.h"
#include "base/memory/weak_ptr.h"
#include "base/time/tick_clock.h"
#include "base/timer/timer.h"
#include "components/safe_browsing_db/database_manager.h"

namespace safe_browsing {

// This provides a simpler interface to
// SafeBrowsingDatabaseManager::CheckCsdWhitelistUrl() for callers that
// don't want to track their own clients.

class WhitelistCheckerClient : public SafeBrowsingDatabaseManager::Client {
 public:
  // Static method to instantiat and start a check. The callback will
  // be invoked when it's done or if database_manager gets shut down.
  // Call on IO thread.
  static void StartCheckCsdWhitelist(
      scoped_refptr<SafeBrowsingDatabaseManager> database_manager,
      const GURL& url,
      base::Callback<void(bool)> callback_for_result);

  WhitelistCheckerClient(base::Callback<void(bool)> callback_for_result);
  ~WhitelistCheckerClient() override;

  // SafeBrowsingDatabaseMananger::Client impl
  void OnCheckWhitelistUrlResult(bool is_whitelisted) override;

  // If set to a non-null ptr, this is used for the timeout timer.
  static void SetClockForTesting(base::TickClock* TickClock);

 protected:
  static const int kTimeoutMsec = 5000;
  base::OneShotTimer timer_;
  base::Callback<void(bool)> callback_for_result_;
  base::WeakPtrFactory<WhitelistCheckerClient> weak_factory_;

 private:
  WhitelistCheckerClient();
};

}  // namespace safe_browsing

#endif  // COMPONENTS_SAFE_BROWSING_DB_WHITELIST_CHECKER_CLIENT_H_
