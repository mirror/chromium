// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "android_webview/browser/aw_safe_browsing_resource_throttle.h"

#include <memory>

#include "android_webview/browser/aw_safe_browsing_whitelist_manager.h"
#include "android_webview/browser/aw_url_checker_delegate_impl.h"
#include "components/safe_browsing/db/v4_protocol_manager_util.h"
#include "net/url_request/url_request.h"

namespace android_webview {

content::ResourceThrottle* MaybeCreateAwSafeBrowsingResourceThrottle(
    net::URLRequest* request,
    content::ResourceType resource_type,
    scoped_refptr<safe_browsing::SafeBrowsingDatabaseManager> database_manager,
    scoped_refptr<AwSafeBrowsingUIManager> ui_manager,
    AwSafeBrowsingWhitelistManager* whitelist_manager) {
  if (!database_manager->IsSupported())
    return nullptr;

  return new AwSafeBrowsingParallelResourceThrottle(
      request, resource_type, std::move(database_manager),
      std::move(ui_manager), whitelist_manager);
}

AwSafeBrowsingParallelResourceThrottle::AwSafeBrowsingParallelResourceThrottle(
    net::URLRequest* request,
    content::ResourceType resource_type,
    scoped_refptr<safe_browsing::SafeBrowsingDatabaseManager> database_manager,
    scoped_refptr<AwSafeBrowsingUIManager> ui_manager,
    AwSafeBrowsingWhitelistManager* whitelist_manager)
    : safe_browsing::BaseParallelResourceThrottle(
          request,
          resource_type,
          new AwUrlCheckerDelegateImpl(std::move(database_manager),
                                       std::move(ui_manager),
                                       whitelist_manager)) {}

AwSafeBrowsingParallelResourceThrottle::
    ~AwSafeBrowsingParallelResourceThrottle() = default;

}  // namespace android_webview
