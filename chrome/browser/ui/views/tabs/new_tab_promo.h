// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_TABS_NEW_TAB_PROMO_H_
#define CHROME_BROWSER_UI_VIEWS_TABS_NEW_TAB_PROMO_H_

#include "chrome/browser/ui/views/tabs/new_tab_button.h"
#include "ui/views/bubble/bubble_dialog_delegate.h"

class NewTabButton;

// View used to display New Tab Promo when it has been indicated that the user
// needs to see it. This will be triggered by the NewTabButton when promoted by
// NewTabFeatureEngagementTracker.
class NewTabPromo : public views::BubbleDialogDelegateView {
 public:
  // Constructs NewTabPromo. Anchors the bubble to the NewTabButton.
  // The bubble will auto-close.
  explicit NewTabPromo(NewTabButton* new_tab_button,
                       const gfx::Rect anchor_rect);
  ~NewTabPromo() override;

  // Closes the bubble.
  void CloseBubble();

  // Notifies the NewTabButton that the promo has closed.
  void NotifyNewTabButtonOnClose();

 private:
  // View:
  void OnMouseEntered(const ui::MouseEvent& event) override;
  void OnMouseExited(const ui::MouseEvent& event) override;
  bool OnMousePressed(const ui::MouseEvent& event) override;

  // WidgetObserver overrides:
  void OnWidgetDestroying(views::Widget* widget) override;

  // BubbleDialogDelegateView:
  void Init() override;
  int GetDialogButtons() const override;

  // Starts a timer which will close the bubble.
  void StartAutoCloseTimer();

  // Stops the auto-close timer.
  void StopAutoCloseTimer();

  // Timer used to auto close the bubble.
  base::OneShotTimer timer_;

  // Timer duration that is made shorter if a user hovers over the button.
  base::TimeDelta auto_close_duration_;

  // The NewTabButton that the promo is anchored to. Owner of the
  // NewTabPromo.
  NewTabButton* const new_tab_button_;
};

#endif  // CHROME_BROWSER_UI_VIEWS_TABS_NEW_TAB_PROMO_H_
