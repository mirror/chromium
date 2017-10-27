// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_WEBUI_INTERSTITIALS_INTERSTITIAL_UI_H_
#define IOS_CHROME_BROWSER_UI_WEBUI_INTERSTITIALS_INTERSTITIAL_UI_H_

#include "base/macros.h"
#include "ios/web/public/webui/web_ui_ios_controller.h"

// The WebUI handler for chrome://interstitials.
class InterstitialsUI : public web::WebUIIOSController {
 public:
  explicit InterstitialsUI(web::WebUIIOS* web_ui);
  ~InterstitialsUI() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(InterstitialsUI);
};

#endif  // IOS_CHROME_BROWSER_UI_WEBUI_INTERSTITIALS_INTERSTITIAL_UI_H_
