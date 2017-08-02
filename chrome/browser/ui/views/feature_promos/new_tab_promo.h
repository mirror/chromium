// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_FEATURE_PROMOS_NEW_TAB_PROMO_H_
#define CHROME_BROWSER_UI_VIEWS_FEATURE_PROMOS_NEW_TAB_PROMO_H_

#include "base/macros.h"
#include "chrome/browser/ui/views/feature_promos/feature_promo.h"

// The NewTabPromo is a bubble anchored to the right of the New Tab Button. It
// draws users attention to the newtab button. It is called by the New Tab
// Button when prompted by the NewTabTracker.
class NewTabPromoBubbleView : public FeaturePromoBubbleView {
 public:
  // FeaturePromoBubbleView:
  static NewTabPromoBubbleView* Create(views::View* anchor_view,
                                       const gfx::Rect& anchor_rect);

 private:
  // Constructs NewTabPromo. Anchors the bubble to the NewTabButton.
  // The bubble widget and promo are owned by their native widget.
  explicit NewTabPromoBubbleView(const gfx::Rect& anchor_rect);
  ~NewTabPromoBubbleView() override;

  // FeaturePromoBubbleView:
  int GetStringSpecifier() override;

  DISALLOW_COPY_AND_ASSIGN(NewTabPromoBubbleView);
};

#endif  // CHROME_BROWSER_UI_VIEWS_FEATURE_PROMOS_NEW_TAB_PROMO_H_
