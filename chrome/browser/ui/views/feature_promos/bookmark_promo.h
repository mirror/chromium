// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_FEATURE_PROMOS_BOOKMARK_PROMO_H_
#define CHROME_BROWSER_UI_VIEWS_FEATURE_PROMOS_BOOKMARK_PROMO_H_

#include "base/macros.h"
#include "chrome/browser/ui/views/feature_promos/feature_promo.h"

// The BookmarkPromoBubbleView is a bubble anchored to the left of the Bookmark
// Button to draw attention. It is called by the StarView when
// prompted by the feature_engagement::BookmarkTracker.
class BookmarkPromoBubbleView : public FeaturePromoBubbleView {
 public:
  // FeaturePromoBubbleView:
  static BookmarkPromoBubbleView* Create(views::View* anchor_view);

 private:
  // Anchors the BookmarkPromoBubbleView to the StarView.
  // The bubble widget and promo are owned by their native widget.
  explicit BookmarkPromoBubbleView(views::View* anchor_view);
  ~BookmarkPromoBubbleView() override;

  // FeaturePromoBubbleView:
  int GetStringSpecifier() override;

  DISALLOW_COPY_AND_ASSIGN(BookmarkPromoBubbleView);
};

#endif  // CHROME_BROWSER_UI_VIEWS_FEATURE_PROMOS_BOOKMARK_PROMO_H_
