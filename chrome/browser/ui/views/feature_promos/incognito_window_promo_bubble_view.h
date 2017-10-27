// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_FEATURE_PROMOS_INCOGNITO_WINDOW_PROMO_BUBBLE_VIEW_H_
#define CHROME_BROWSER_UI_VIEWS_FEATURE_PROMOS_INCOGNITO_WINDOW_PROMO_BUBBLE_VIEW_H_

#include "base/macros.h"
#include "chrome/browser/feature_engagement/feature_promo_bubble.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/views/feature_promos/feature_promo_bubble_view.h"
#include "ui/views/widget/widget_observer.h"

class AppMenuButton;

// The IncognitoWindowPromoBubbleView is a bubble anchored to the right of the
// App Menu Button. It draws users' attention to the App Menu Button. It is
// created by the App Menu Button when prompted by the IncognitoWindowTracker.
// It is owned by its own native widget.
class IncognitoWindowPromoBubbleView
    : public feature_engagement::FeaturePromoBubbleView {
 public:
  // Returns a raw pointer that is owned by its native widget.
  static IncognitoWindowPromoBubbleView* CreateOwned(views::View* anchor_view);

  AppMenuButton* app_menu_button() { return app_menu_button_; }
  void set_app_menu_button(AppMenuButton* app_menu_button) {
    app_menu_button_ = app_menu_button;
  }

  // FeaturePromoBubble:
  void ClosePromoBubble() override;

  // feature_engagement::FeaturePromoBubbleView:
  void ShowPromoBubble() override;

 private:
  // Anchors the bubble to |anchor_view|. The bubble widget and promo are
  // owned by their native widget.
  explicit IncognitoWindowPromoBubbleView(views::View* anchor_view);
  ~IncognitoWindowPromoBubbleView() override;

  // Returns the string ID to display in the promo.
  int GetStringSpecifier();

  // views::WidgetObserver:
  void OnWidgetDestroying(views::Widget* widget) override;

  IncognitoWindowPromoBubbleView* incognito_promo_ = nullptr;

  AppMenuButton* app_menu_button_ = nullptr;

  // Observes the |incognito_promo_|'s Widget. Used to tell whether the promo
  // is open and is called back when it closes.
  ScopedObserver<views::Widget, WidgetObserver> incognito_promo_observer_;

  DISALLOW_COPY_AND_ASSIGN(IncognitoWindowPromoBubbleView);
};

#endif  // CHROME_BROWSER_UI_VIEWS_FEATURE_PROMOS_INCOGNITO_WINDOW_PROMO_BUBBLE_VIEW_H_
