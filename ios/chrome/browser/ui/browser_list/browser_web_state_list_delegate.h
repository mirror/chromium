// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_BROWSER_LIST_BROWSER_WEB_STATE_LIST_DELEGATE_H_
#define IOS_CHROME_BROWSER_UI_BROWSER_LIST_BROWSER_WEB_STATE_LIST_DELEGATE_H_

#include <memory>
#include <vector>

#include "base/macros.h"
#import "ios/chrome/browser/ui/browser_list/browser_web_state_helper.h"
#import "ios/chrome/browser/web_state_list/web_state_list_delegate.h"

class Browser;

// WebStateList delegate for the new architecture.
class BrowserWebStateListDelegate : public WebStateListDelegate {
 public:
  explicit BrowserWebStateListDelegate(Browser* browser);
  ~BrowserWebStateListDelegate() override;

  // Adds |helper| to |web_state_helpers_|, executing its OnWebStateAdded() on
  // all WebStates in |web_state_list_|.
  void AddWebStateHelper(std::unique_ptr<BrowserWebStateHelper> helper);

 private:
  // WebStateListDelegate:
  void WillAddWebState(web::WebState* web_state) override;
  void WebStateDetached(web::WebState* web_state) override;

  // The Browser.
  Browser* browser_;
  // The BrowserWebStateHelpers for this Browser.
  std::vector<std::unique_ptr<BrowserWebStateHelper>> web_state_helpers_;

  DISALLOW_COPY_AND_ASSIGN(BrowserWebStateListDelegate);
};

#endif  // IOS_CHROME_BROWSER_UI_BROWSER_LIST_BROWSER_WEB_STATE_LIST_DELEGATE_H_
