// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/ui/webui/suggestions_ui.h"

#include <map>
#include <string>

#include "components/suggestions/suggestions_ui_internals.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/chrome_url_constants.h"
#include "ios/chrome/browser/suggestions/suggestions_service_factory.h"
#include "ios/web/public/url_data_source_ios.h"

namespace suggestions {

namespace {

// Glues a SuggestionsSource instance to //ios/chrome.
class SuggestionsSourceWrapper : public web::URLDataSourceIOS {
 public:
  explicit SuggestionsSourceWrapper(ios::ChromeBrowserState* browser_state);

  // web::URLDataSourceIOS implementation.
  std::string GetSource() const override;
  void StartDataRequest(
      const std::string& path,
      const web::URLDataSourceIOS::GotDataCallback& callback) override;
  std::string GetMimeType(const std::string& path) const override;

 private:
  ~SuggestionsSourceWrapper() override;

  // Only used when servicing requests on the UI thread.
  ios::ChromeBrowserState* const browser_state_;
  SuggestionsSource suggestions_source_;

  DISALLOW_COPY_AND_ASSIGN(SuggestionsSourceWrapper);
};

SuggestionsSourceWrapper::SuggestionsSourceWrapper(
    ios::ChromeBrowserState* browser_state)
    : browser_state_(browser_state),
      suggestions_source_(
          SuggestionsServiceFactory::GetForBrowserState(browser_state_),
          kChromeUISuggestionsURL) {}

SuggestionsSourceWrapper::~SuggestionsSourceWrapper() {}

std::string SuggestionsSourceWrapper::GetSource() const {
  return kChromeUISuggestionsHost;
}

void SuggestionsSourceWrapper::StartDataRequest(
    const std::string& path,
    const web::URLDataSourceIOS::GotDataCallback& callback) {
  suggestions_source_.StartDataRequest(path, callback);
}

std::string SuggestionsSourceWrapper::GetMimeType(
    const std::string& path) const {
  return "text/html";
}

}  // namespace

SuggestionsUI::SuggestionsUI(web::WebUIIOS* web_ui)
    : web::WebUIIOSController(web_ui) {
  ios::ChromeBrowserState* browser_state =
      ios::ChromeBrowserState::FromWebUIIOS(web_ui);
  web::URLDataSourceIOS::Add(browser_state,
                             new SuggestionsSourceWrapper(browser_state));
}

SuggestionsUI::~SuggestionsUI() {}

}  // namespace suggestions
