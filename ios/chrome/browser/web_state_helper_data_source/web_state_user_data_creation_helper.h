// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_WEB_STATE_HELPER_DATA_SOURCE_WEB_STATE_USER_DATA_CREATION_HELPER_H_
#define IOS_CHROME_BROWSER_WEB_STATE_HELPER_DATA_SOURCE_WEB_STATE_USER_DATA_CREATION_HELPER_H_

#include "ios/chrome/browser/ui/browser_list/browser_web_state_helper.h"

// BrowserWebStateHelper subclass that creates WebStateUserData objects.
class WebStateUserDataCreationHelper : public BrowserWebStateHelper {
 public:
  explicit WebStateUserDataCreationHelper() = default;
  ~WebStateUserDataCreationHelper() override = default;

 private:
  // BrowserWebStateHelper:
  void OnWebStateAdded(web::WebState* web_state) override;
};

#endif  // IOS_CHROME_BROWSER_WEB_STATE_HELPER_DATA_SOURCE_WEB_STATE_USER_DATA_CREATION_HELPER_H_
