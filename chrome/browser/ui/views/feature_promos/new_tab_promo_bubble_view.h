// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_FEATURE_PROMOS_NEW_TAB_PROMO_BUBBLE_VIEW_H_
#define CHROME_BROWSER_UI_VIEWS_FEATURE_PROMOS_NEW_TAB_PROMO_BUBBLE_VIEW_H_

#include "base/macros.h"
#include "chrome/browser/feature_engagement/feature_promo_bubble.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/views/feature_promos/feature_promo_bubble_view.h"

// The NewTabPromoBubbleView is a bubble anchored to the right of the New Tab
// Button. It draws users' attention to the New Tab Button. It is created by
// the New Tab Button when prompted by the NewTabTracker.
// It is owned by its own native widget.
class NewTabPromoBubbleView
    : public feature_engagement::FeaturePromoBubbleView {
 public:
  // Returns a raw pointer that is owned by its native widget.
  static NewTabPromoBubbleView* CreateOwned(views::View* anchor_view);

  // FeaturePromoBubble:
  void ShowPromoBubble(Browser* browser) override;
  void ClosePromoBubble() override;

 private:
  // Anchors the bubble to |anchor_view|. The bubble widget and promo are
  // owned by their native widget.
  explicit NewTabPromoBubbleView(views::View* anchor_view);
  ~NewTabPromoBubbleView() override;

  // Returns the string ID to display in the promo.
  static int GetStringSpecifier();

  // views::WidgetObserver:
  void OnWidgetDestroying(views::Widget* widget) override;

  NewTabPromoBubbleView* new_tab_promo_ = nullptr;

  // Observes the |new_tab_promo_|'s Widget. Used to tell whether the promo
  // is open and is called back when it closes.
  ScopedObserver<views::Widget, WidgetObserver> new_tab_promo_observer_;

  DISALLOW_COPY_AND_ASSIGN(NewTabPromoBubbleView);
};

#endif  // CHROME_BROWSER_UI_VIEWS_FEATURE_PROMOS_NEW_TAB_PROMO_BUBBLE_VIEW_H_
