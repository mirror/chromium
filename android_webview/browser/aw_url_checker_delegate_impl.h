// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ANDROID_WEBVIEW_BROWSER_AW_URL_CHECKER_DELEGATE_IMPL_H_
#define ANDROID_WEBVIEW_BROWSER_AW_URL_CHECKER_DELEGATE_IMPL_H_

#include "android_webview/browser/aw_safe_browsing_ui_manager.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "components/safe_browsing/browser/url_checker_delegate.h"
#include "components/safe_browsing_db/database_manager.h"

namespace android_webview {

class AwUrlCheckerDelegateImpl : public safe_browsing::UrlCheckerDelegate {
 public:
  AwUrlCheckerDelegateImpl(
      scoped_refptr<safe_browsing::SafeBrowsingDatabaseManager>
          database_manager,
      scoped_refptr<AwSafeBrowsingUIManager> ui_manager);

 private:
  ~AwUrlCheckerDelegateImpl() override;

  // Implementation of UrlCheckerDelegate:
  void MaybeDestroyPrerenderContents(
      const base::Callback<content::WebContents*()>& web_contents_getter)
      override;
  void StartDisplayingBlockingPageHelper(
      const security_interstitials::UnsafeResource& resource) override;
  const safe_browsing::SBThreatTypeSet& GetThreatTypes() override;
  safe_browsing::SafeBrowsingDatabaseManager* GetDatabaseManager() override;
  safe_browsing::BaseUIManager* GetUIManager() override;

  scoped_refptr<safe_browsing::SafeBrowsingDatabaseManager> database_manager_;
  scoped_refptr<AwSafeBrowsingUIManager> ui_manager_;
  safe_browsing::SBThreatTypeSet threat_types_;

  DISALLOW_COPY_AND_ASSIGN(AwUrlCheckerDelegateImpl);
};

}  // namespace android_webview

#endif  // ANDROID_WEBVIEW_BROWSER_AW_URL_CHECKER_DELEGATE_IMPL_H_
