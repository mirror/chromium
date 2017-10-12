// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_FEATURE_ENGAGEMENT_FEATURE_PROMO_BUBBLE_H_
#define CHROME_BROWSER_FEATURE_ENGAGEMENT_FEATURE_PROMO_BUBBLE_H_

#include "chrome/browser/ui/toolbar/app_menu_icon_controller.h"

class FeaturePromoBubble {
 public:
  FeaturePromoBubble() {}
  virtual ~FeaturePromoBubble() {}

  virtual void ShowPromoBubble() = 0;

  virtual void ClosePromoBubble() = 0;
};

namespace feature_engagement {

// Gets the severity of app menu button of the last active browser.
AppMenuIconController::Severity GetAppMenuButtonSeverity();

// Shows an incognito window in product help feature promo bubble.
FeaturePromoBubble* CreateIncognitoFeaturePromoBubble();

// Create a new tab in product help feature promo bubble.
FeaturePromoBubble* CreateNewTabFeaturePromoBubble();
}  // namespace feature_engagement

#endif  // CHROME_BROWSER_FEATURE_ENGAGEMENT_FEATURE_PROMO_BUBBLE_H_
