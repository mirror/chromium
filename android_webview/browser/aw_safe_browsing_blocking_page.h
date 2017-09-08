// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ANDROID_WEBVIEW_BROWSER_AW_SAFE_BROWSING_BLOCKING_PAGE_H_
#define ANDROID_WEBVIEW_BROWSER_AW_SAFE_BROWSING_BLOCKING_PAGE_H_

#include "components/safe_browsing/base_blocking_page.h"
#include "components/security_interstitials/core/base_safe_browsing_error_ui.h"

class PrefService;

namespace security_interstitials {
struct UnsafeResource;
}  // namespace security_interstitials

namespace android_webview {

class AwSafeBrowsingBlockingPageFactory;
class AwSafeBrowsingUIManager;

class AwSafeBrowsingBlockingPage : public safe_browsing::BaseBlockingPage {
 public:
  typedef security_interstitials::UnsafeResource UnsafeResource;

  static void ShowBlockingPage(AwSafeBrowsingUIManager* ui_manager,
                               const UnsafeResource& unsafe_resource,
                               PrefService* pref_service);

  // Makes the passed |factory| the factory used to instantiate
  // SafeBrowsingBlockingPage objects. Useful for tests.
  static void RegisterFactory(AwSafeBrowsingBlockingPageFactory* factory) {
    factory_ = factory;
  }

 protected:
  friend class AwSafeBrowsingBlockingPageFactory;
  friend class AwSafeBrowsingBlockingPageFactoryImpl;
  //  friend class AwSafeBrowsingUIManagerTest;
  friend class TestAwSafeBrowsingBlockingPageFactory;

  // Used to specify which BaseSafeBrowsingErrorUI to instantiate, and
  // parameters they require.
  // GENERATED_JAVA_ENUM_PACKAGE: org.chromium.android_webview
  enum class ErrorUiType { LOUD, QUIET_SMALL, QUIET_GIANT };

  // Don't instantiate this class directly, use ShowBlockingPage instead.
  AwSafeBrowsingBlockingPage(
      AwSafeBrowsingUIManager* ui_manager,
      content::WebContents* web_contents,
      PrefService* pref_service,
      const GURL& main_frame_url,
      const UnsafeResourceList& unsafe_resources,
      const BaseSafeBrowsingErrorUI::SBErrorDisplayOptions& display_options,
      const AwSafeBrowsingBlockingPage::ErrorUiType errorType);

  // Called when the interstitial is going away. If there is a
  // pending threat details object, we look at the user's
  // preferences, and if the option to send threat details is
  // enabled, the report is scheduled to be sent on the |ui_manager_|.
  void FinishThreatDetails(const base::TimeDelta& delay,
                           bool did_proceed,
                           int num_visits) override;

  // Creates a blocking page. Use ShowBlockingPage if you don't need to access
  // the blocking page directly.
  static AwSafeBrowsingBlockingPage* CreateBlockingPage(
      AwSafeBrowsingUIManager* ui_manager,
      content::WebContents* web_contents,
      PrefService* pref_service,
      const GURL& main_frame_url,
      const UnsafeResource& unsafe_resource);

  // Whether ThreatDetails collection is in progress as part of this
  // interstitial.
  bool threat_details_in_progress_;

  // The factory used to instantiate AwSafeBrowsingBlockingPage objects.
  // Useful for tests, so they can provide their own implementation of
  // AwSafeBrowsingBlockingPage.
  static AwSafeBrowsingBlockingPageFactory* factory_;
};

// Factory for creating AwSafeBrowsingBlockingPage. Useful for tests.
class AwSafeBrowsingBlockingPageFactory {
 public:
  virtual ~AwSafeBrowsingBlockingPageFactory() {}
  virtual AwSafeBrowsingBlockingPage* CreateAwSafeBrowsingPage(
      AwSafeBrowsingUIManager* ui_manager,
      content::WebContents* web_contents,
      PrefService* pref_service,
      const GURL& main_frame_url,
      const AwSafeBrowsingBlockingPage::UnsafeResourceList& unsafe_resources,
      const AwSafeBrowsingBlockingPage::ErrorUiType errorType) = 0;
};

}  // namespace android_webview

#endif  // ANDROID_WEBVIEW_BROWSER_AW_SAFE_BROWSING_BLOCKING_PAGE_H_
