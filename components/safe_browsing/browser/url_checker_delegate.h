// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SAFE_BROWSING_BROWSER_URL_CHECKER_DELEGATE_H_
#define COMPONENTS_SAFE_BROWSING_BROWSER_URL_CHECKER_DELEGATE_H_

#include "base/callback.h"
#include "base/memory/ref_counted.h"
#include "components/safe_browsing_db/v4_protocol_manager_util.h"

namespace content {
class WebContents;
}

namespace security_interstitials {
struct UnsafeResource;
}

namespace safe_browsing {

class BaseUIManager;
class SafeBrowsingDatabaseManager;

// All methods should only be called from the IO thread.
class UrlCheckerDelegate
    : public base::RefCountedThreadSafe<UrlCheckerDelegate> {
 public:
  // Overrides this to destroy prerender contents in chrome.
  virtual void MaybeDestroyPrerenderContents(
      const base::Callback<content::WebContents*()>& web_contents_getter) {}

  // Posts a task for StartDisplayingBlockingPage
  virtual void StartDisplayingBlockingPageHelper(
      const security_interstitials::UnsafeResource& resource) {}

  virtual const SBThreatTypeSet& GetThreatTypes() = 0;
  virtual SafeBrowsingDatabaseManager* GetDatabaseManager() = 0;
  virtual BaseUIManager* GetUIManager() = 0;

 protected:
  friend class base::RefCountedThreadSafe<UrlCheckerDelegate>;
  virtual ~UrlCheckerDelegate() {}
};

}  // namespace safe_browsing

#endif  // COMPONENTS_SAFE_BROWSING_BROWSER_URL_CHECKER_DELEGATE_H_
