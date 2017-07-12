// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "android_webview/browser/aw_url_checker_delegate_impl.h"

#include "base/bind.h"
#include "components/safe_browsing_db/v4_protocol_manager_util.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/web_contents.h"

namespace android_webview {
namespace {

void StartDisplayingBlockingPage(
    scoped_refptr<safe_browsing::BaseUIManager> ui_manager,
    const security_interstitials::UnsafeResource& resource) {
  content::WebContents* web_contents = resource.web_contents_getter.Run();
  if (web_contents) {
    ui_manager->DisplayBlockingPage(resource);
    return;
  }

  // Tab is gone or it's being prerendered.
  content::BrowserThread::PostTask(content::BrowserThread::IO, FROM_HERE,
                                   base::Bind(resource.callback, false));
}

}  // namespace

AwUrlCheckerDelegateImpl::AwUrlCheckerDelegateImpl(
    scoped_refptr<safe_browsing::SafeBrowsingDatabaseManager> database_manager,
    scoped_refptr<AwSafeBrowsingUIManager> ui_manager)
    : database_manager_(std::move(database_manager)),
      ui_manager_(std::move(ui_manager)),
      threat_types_(safe_browsing::CreateSBThreatTypeSet(
          {safe_browsing::SB_THREAT_TYPE_URL_MALWARE,
           safe_browsing::SB_THREAT_TYPE_URL_PHISHING})) {}

AwUrlCheckerDelegateImpl::~AwUrlCheckerDelegateImpl() = default;

void AwUrlCheckerDelegateImpl::MaybeDestroyPrerenderContents(
    const base::Callback<content::WebContents*()>& web_contents_getter) {}

void AwUrlCheckerDelegateImpl::StartDisplayingBlockingPageHelper(
    const security_interstitials::UnsafeResource& resource) {
  // TODO(yzshen): Figure out how to hook up with
  // WebViewClient#onSafeBrowsingHit(). See
  // android_webview/browser/aw_safe_browsing_resource_throttle.{h,cc}
  content::BrowserThread::PostTask(
      content::BrowserThread::UI, FROM_HERE,
      base::BindOnce(&StartDisplayingBlockingPage, ui_manager_, resource));
}

const safe_browsing::SBThreatTypeSet&
AwUrlCheckerDelegateImpl::GetThreatTypes() {
  return threat_types_;
}

safe_browsing::SafeBrowsingDatabaseManager*
AwUrlCheckerDelegateImpl::GetDatabaseManager() {
  return database_manager_.get();
}

safe_browsing::BaseUIManager* AwUrlCheckerDelegateImpl::GetUIManager() {
  return ui_manager_.get();
}

}  // namespace android_webview
