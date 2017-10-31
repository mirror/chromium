// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/ui/webui/interstitial_ui.h"

#include "base/callback.h"
#include "base/mac/bind_objc_block.h"
#include "base/memory/ref_counted_memory.h"
#include "components/captive_portal/captive_portal_detector.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/chrome_url_constants.h"
#import "ios/chrome/browser/ssl/ios_captive_portal_blocking_page.h"
#include "ios/chrome/browser/ssl/ios_ssl_error_handler.h"
#include "ios/web/public/url_data_source_ios.h"
#import "ios/web/public/web_state/web_state.h"
#include "ios/web/public/web_state/web_state_observer.h"
#include "ios/web/public/web_thread.h"
#include "ios/web/public/webui/web_ui_ios.h"
#include "url/gurl.h"

namespace {
// Empty callback.
void DoNothing(bool proceed) {}

IOSCaptivePortalBlockingPage* CreateCaptivePortalBlockingPage(
    web::WebState* web_state) {
  GURL request_url = GURL(kChromeUIInterstitialsHost);
  GURL landing_url = GURL(captive_portal::CaptivePortalDetector::kDefaultURL);
  return new IOSCaptivePortalBlockingPage(web_state, request_url, landing_url,
                                          base::Bind(&DoNothing);
}

}  // namespace

// Implementation of chrome://interstitials demonstration pages. This code is
// not used to present interstitials to the user in real situations.
class InterstitialHtmlSource : public web::WebStateObserver,
                               public web::URLDataSourceIOS {
 public:
  explicit InterstitialHtmlSource(web::WebState* web_state);

  // web::WebStateObserver implementation.
  void DidStopLoading() override;

  // web::URLDataSourceIOS implementation.
  std::string GetSource() const override;
  void StartDataRequest(
      const std::string& path,
      const web::URLDataSourceIOS::GotDataCallback& callback) override;
  std::string GetMimeType(const std::string& path) const override;

 private:
  ~InterstitialHtmlSource() override;

  // The web state associated with this interstitial.
  web::WebState* web_state_;
  // The displayed captive portal blocking page.
  IOSCaptivePortalBlockingPage* captive_portal_blocking_page_;
  // If set, loads a dummy webpage after the next completed page load. This fake
  // page load is required because interstitials can not be shown if the
  // web_state has only loaded chrome:// pages.
  bool non_chrome_pageload_needed_;
  // If set, displays the captive portal blocking page after the next completed
  // page load.
  bool captive_portal_blocking_page_needs_display_;

  DISALLOW_COPY_AND_ASSIGN(InterstitialHtmlSource);
};

InterstitialHtmlSource::InterstitialHtmlSource(web::WebState* web_state)
    : web::WebStateObserver(web_state), web_state_(web_state) {}

InterstitialHtmlSource::~InterstitialHtmlSource() {}

void InterstitialHtmlSource::DidStopLoading() {
  if (non_chrome_pageload_needed_) {
    non_chrome_pageload_needed_ = false;
    captive_portal_blocking_page_needs_display_ = true;
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

  if (captive_portal_blocking_page_needs_display_) {
    captive_portal_blocking_page_needs_display_ = false;
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

std::string InterstitialHtmlSource::GetSource() const {
  return kChromeUIInterstitialsHost;
}

void InterstitialHtmlSource::StartDataRequest(
    const std::string& path,
    const web::URLDataSourceIOS::GotDataCallback& callback) {
  std::string html;
  // Using this form of the path to do exact matching, while ignoring the query
  // (everything after the ? character).
  GURL url_without_query =
      GURL(kChromeUIInterstitialsURL).GetWithEmptyPath().Resolve(path);
  std::string path_without_query = url_without_query.path();
  if (path_without_query == "/captiveportal") {
    // The captive portal interstitial must be presented from a web_state which
    // has loaded a "real" (non chrome://) webpage. In order to satisfy this
    // requirement, load a static webpage with "http://" scheme.
    non_chrome_pageload_needed_ = true;
    // This HTML must not be empty, but is not important as the
    // |non_chrome_pageload_needed_| flag will trigger an immediate load of
    // another page.
    html = "/captiveportal";
  }

  std::string html_copy(html);
  callback.Run(base::RefCountedString::TakeString(&html_copy));
}

std::string InterstitialHtmlSource::GetMimeType(const std::string& path) const {
  return "text/html";
}

// InterstitialsUI

InterstitialsUI::InterstitialsUI(web::WebUIIOS* web_ui)
    : web::WebUIIOSController(web_ui) {
  // Set up the chrome://interstitials/ source.
  web::WebState* web_state = web_ui->GetWebState();
  web::URLDataSourceIOS::Add(ios::ChromeBrowserState::FromWebUIIOS(web_ui),
                             new InterstitialHtmlSource(web_state));
}

InterstitialsUI::~InterstitialsUI() {}
