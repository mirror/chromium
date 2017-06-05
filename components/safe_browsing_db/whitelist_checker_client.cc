// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
#include "components/safe_browsing_db/whitelist_checker_client.h"

#include "base/bind.h"
#include "content/public/browser/browser_thread.h"

namespace safe_browsing {

namespace {

// Used for testing
base::TickClock* g_whitelist_tick_clock = nullptr;

}  // namespace

using content::BrowserThread;

// Static
void WhitelistCheckerClient::StartCheckCsdWhitelist(
    scoped_refptr<SafeBrowsingDatabaseManager> database_manager,
    const GURL& url,
    base::Callback<void(bool)> callback_for_result) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  // TODO(nparker): Maybe also call SafeBrowsingDatabaseManager::CanCheckUrl()
  if (!url.is_valid()) {
    callback_for_result.Run(true);  // Is whitelisted
    return;
  }

  // Make a client for each request. We could have several in flight at once.
  std::unique_ptr<WhitelistCheckerClient> client =
      base::MakeUnique<WhitelistCheckerClient>(callback_for_result);
  AsyncMatch match = database_manager->CheckCsdWhitelistUrl(url, client.get());

  switch (match) {
    case AsyncMatch::MATCH:
      callback_for_result.Run(true /* is_whitelisted */);
      break;
    case AsyncMatch::NO_MATCH:
      callback_for_result.Run(false /* is_whitelisted */);
      break;
    case AsyncMatch::ASYNC:
      // Client is now self-owned. When it gets called back with the result,
      // it will delete itself.
      client.release();
      break;
  }
}

WhitelistCheckerClient::WhitelistCheckerClient(
    base::Callback<void(bool)> callback_for_result)
    : timer_(g_whitelist_tick_clock),
      callback_for_result_(callback_for_result),
      weak_factory_(this) {
  // Set a timer to fail open, i.e. call it "whitelisted", if the full
  // check takes too long.
  // This will be canceled when *this is destructed.
  auto timeout_callback =
      base::Bind(&WhitelistCheckerClient::OnCheckWhitelistUrlResult,
                 weak_factory_.GetWeakPtr(), true /* is_whitelisted */);
  timer_.Start(FROM_HERE, base::TimeDelta::FromMilliseconds(kTimeoutMsec),
               timeout_callback);
}

// static
void WhitelistCheckerClient::SetClockForTesting(base::TickClock* tick_clock) {
  g_whitelist_tick_clock = tick_clock;
}

WhitelistCheckerClient::~WhitelistCheckerClient() {}

// SafeBrowsingDatabaseMananger::Client impl
void WhitelistCheckerClient::OnCheckWhitelistUrlResult(bool is_whitelisted) {
  // This method is invoked only if we're already self-owned.
  callback_for_result_.Run(is_whitelisted);
  delete this;
}

}  // namespace safe_browsing
