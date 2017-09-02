// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_BROWSER_LIST_BROWSER_WEB_STATE_HELPER_H_
#define IOS_CHROME_BROWSER_UI_BROWSER_LIST_BROWSER_WEB_STATE_HELPER_H_

namespace web {
class WebState;
}

// BrowserWebStateHelpers are used to execute tasks on a WebState when it is
// added or removed from the Browser's WebStateList.
class BrowserWebStateHelper {
 public:
  BrowserWebStateHelper() = default;
  virtual ~BrowserWebStateHelper() = default;

  // Called when |web_state| is added to the WebStateList.
  virtual void OnWebStateAdded(web::WebState* web_state) {}
  // Called when |web_state| was removed from the WebStateList.
  virtual void OnWebStateDetached(web::WebState* web_state) {}
};

#endif  // IOS_CHROME_BROWSER_UI_BROWSER_LIST_BROWSER_WEB_STATE_HELPER_H_
