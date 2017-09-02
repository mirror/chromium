// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_WEB_STATE_HELPER_DATA_SOURCE_BROWSER_WEB_STATE_DELEGATE_HELPER_H_
#define IOS_CHROME_BROWSER_WEB_STATE_HELPER_DATA_SOURCE_BROWSER_WEB_STATE_DELEGATE_HELPER_H_

#include "base/macros.h"
#include "ios/chrome/browser/ui/browser_list/browser_web_state_helper.h"

class Browser;
namespace web {
class WebStateDelegate;
}

// BrowserWebStateHelper subclass that sets the WebStates' delegate.
class BrowserWebStateDelegateHelper : public BrowserWebStateHelper {
 public:
  // Constructor for a helper that uses |browser|'s BrowserWebStateDelegate as
  // the delegate for the WebStates is receives.
  explicit BrowserWebStateDelegateHelper(Browser* browser);
  ~BrowserWebStateDelegateHelper() override;

 private:
  // BrowserWebStateHelper:
  void OnWebStateAdded(web::WebState* web_state) override;
  void OnWebStateDetached(web::WebState* web_state) override;

  // The delegate to provided to the WebStates.
  web::WebStateDelegate* web_state_delegate_;

  DISALLOW_COPY_AND_ASSIGN(BrowserWebStateDelegateHelper);
};

#endif  // IOS_CHROME_BROWSER_WEB_STATE_HELPER_DATA_SOURCE_BROWSER_WEB_STATE_DELEGATE_HELPER_H_
