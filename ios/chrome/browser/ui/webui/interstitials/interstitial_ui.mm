// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/ui/webui/interstitials/interstitial_ui.h"

#include "base/callback.h"
#include "base/mac/bind_objc_block.h"
#include "base/memory/ref_counted_memory.h"
#include "components/captive_portal/captive_portal_detector.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/chrome_url_constants.h"
#import "ios/chrome/browser/ssl/ios_captive_portal_blocking_page.h"
#import "ios/chrome/browser/ssl/ios_captive_portal_blocking_page_delegate.h"
#include "ios/chrome/browser/ssl/ios_ssl_error_handler.h"
#include "ios/chrome/browser/tabs/legacy_tab_helper.h"
#include "ios/chrome/browser/tabs/tab.h"
#include "ios/web/public/url_data_source_ios.h"
#include "ios/web/public/web_state/web_state.h"
#include "ios/web/public/web_state/web_state_observer.h"
#include "ios/web/public/web_thread.h"
#include "ios/web/public/webui/web_ui_ios.h"
#include "url/gurl.h"

namespace {
IOSCaptivePortalBlockingPage* CreateCaptivePortalBlockingPage(
    web::WebState* web_state) {
  Tab* tab = LegacyTabHelper::GetTabForWebState(web_state);
  id<IOSCaptivePortalBlockingPageDelegate> delegate =
      tab.iOSCaptivePortalBlockingPageDelegate;
  GURL request_url = GURL(kChromeUIInterstitialsHost);
  GURL landing_url = GURL(captive_portal::CaptivePortalDetector::kDefaultURL);

  return new IOSCaptivePortalBlockingPage(web_state, request_url, landing_url,
                                          delegate,
                                          base::BindBlockArc(^(bool proceed){
                                          }));
}

}  // namespace

// Implementation of chrome://interstitials demonstration pages. This code is
// not used in displaying any real interstitials.
class InterstitialHTMLSource : public web::WebStateObserver,
                               public web::URLDataSourceIOS {
 public:
  explicit InterstitialHTMLSource(web::WebState* web_state);

  // web::WebStateObserver implementation.
  void DidStopLoading() override;

  // web::URLDataSourceIOS implementation.
  std::string GetSource() const override;
  void StartDataRequest(
      const std::string& path,
      const web::URLDataSourceIOS::GotDataCallback& callback) override;
  std::string GetMimeType(const std::string& path) const override;

 private:
  ~InterstitialHTMLSource() override;

  // The web state associated with this interstitial.
  web::WebState* web_state_;
  // The displayed captive portal blocking page.
  IOSCaptivePortalBlockingPage* captive_portal_blocking_page_;
  // Whether or not a page needs to be loaded that doesn't have the chrome://
  // scheme after the current load completes.
  bool needs_real_pageload_;
  // Whether or not the blocking page should be shown after the current load
  // completes.
  bool needs_blocking_page_shown_;

  DISALLOW_COPY_AND_ASSIGN(InterstitialHTMLSource);
};

InterstitialHTMLSource::InterstitialHTMLSource(web::WebState* web_state)
    : web::WebStateObserver(web_state), web_state_(web_state) {}

InterstitialHTMLSource::~InterstitialHTMLSource() {}

void InterstitialHTMLSource::DidStopLoading() {
  if (needs_real_pageload_) {
    needs_real_pageload_ = false;
    needs_blocking_page_shown_ = true;
    web::WebState::OpenURLParams params(
        GURL("http://example.com"), web::Referrer(),
        WindowOpenDisposition::CURRENT_TAB, ui::PAGE_TRANSITION_LINK, false);
    // Delay required because CRWWebController needs to also respond to
    // web::WebStateObserver::DidStopLoading before it is ready for another
    // request.
    web::WebThread::PostTask(web::WebThread::UI, FROM_HERE,
                             base::BindBlockArc(^{
                               web_state_->OpenURL(params);
                             }));
    return;
  }

  if (needs_blocking_page_shown_) {
    needs_blocking_page_shown_ = false;
    // Delay required because CRWWebController needs to also respond to
    // web::WebStateObserver::DidStopLoading before it is ready for another
    // request.
    web::WebThread::PostTask(web::WebThread::UI, FROM_HERE,
                             base::BindBlockArc(^{
                               captive_portal_blocking_page_ =
                                   CreateCaptivePortalBlockingPage(web_state_);
                               captive_portal_blocking_page_->Show();
                             }));
    return;
  }
}

std::string InterstitialHTMLSource::GetSource() const {
  return kChromeUIInterstitialsHost;
}

void InterstitialHTMLSource::StartDataRequest(
    const std::string& path,
    const web::URLDataSourceIOS::GotDataCallback& callback) {
  std::string html;
  // Using this form of the path so we can do exact matching, while ignoring the
  // query (everything after the ? character).
  GURL url = GURL(kChromeUIInterstitialsURL).GetWithEmptyPath().Resolve(path);
  std::string path_without_query = url.path();
  if (path_without_query == "/captiveportal") {
    needs_real_pageload_ = true;
    // This HTML must not be empty, but the interstitial can't be loaded unless
    // this web_state (non-chrome://) page loads, so a pad page will be loaded
    // before the interstial is shown inside DidStopLoading.
    html = "/captiveportal";
  }

  std::string html_copy(html);
  callback.Run(base::RefCountedString::TakeString(&html_copy));
}

std::string InterstitialHTMLSource::GetMimeType(const std::string& path) const {
  return "text/html";
}

// InterstitialsUI

InterstitialsUI::InterstitialsUI(web::WebUIIOS* web_ui)
    : web::WebUIIOSController(web_ui) {
  // Set up the chrome://interstitials/ source.
  web::WebState* web_state = web_ui->GetWebState();
  web::URLDataSourceIOS::Add(ios::ChromeBrowserState::FromWebUIIOS(web_ui),
                             new InterstitialHTMLSource(web_state));
}

InterstitialsUI::~InterstitialsUI(){};
