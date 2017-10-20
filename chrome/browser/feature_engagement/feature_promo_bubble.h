// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_FEATURE_ENGAGEMENT_FEATURE_PROMO_BUBBLE_H_
#define CHROME_BROWSER_FEATURE_ENGAGEMENT_FEATURE_PROMO_BUBBLE_H_

#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/toolbar/app_menu_icon_controller.h"

namespace feature_engagement {

// FeaturePromoBubble is an interface implemented by an object that provides an
// in-product help feature promo bubble widget on the UI.
class FeaturePromoBubble {
 public:
  virtual void ShowPromoBubble(Browser* browser) = 0;
  virtual void ClosePromoBubble() = 0;

 protected:
  virtual ~FeaturePromoBubble() {}
};

// Gets the severity of app menu button associated with |browser|.
AppMenuIconController::Severity GetAppMenuButtonSeverity(Browser* browser);

// Shows an incognito window in product help feature promo bubble.
FeaturePromoBubble* CreateIncognitoFeaturePromoBubble(Browser* browser);

// Create a new tab in product help feature promo bubble.
FeaturePromoBubble* CreateNewTabFeaturePromoBubble(Browser* browser);

}  // namespace feature_engagement

#endif  // CHROME_BROWSER_FEATURE_ENGAGEMENT_FEATURE_PROMO_BUBBLE_H_
