// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_TABS_NEW_TAB_PROMO_H_
#define CHROME_BROWSER_UI_VIEWS_TABS_NEW_TAB_PROMO_H_

#include "base/macros.h"
#include "base/timer/timer.h"
#include "ui/views/bubble/bubble_dialog_delegate.h"

class NewTabButton;
struct gfx::Rect;
struct ui::MouseEvent;
struct views::Widget;

// The NewTabPromo is a floating toast anchored to the right of the newtab
// button. It draws users attention to the newtab button. It is called by the
// NewTab button when prompted by the NewTabFeatureEngagementTracker.
class NewTabPromo : public views::BubbleDialogDelegateView {
 public:
  // Constructs NewTabPromo. Anchors the bubble to the NewTabButton.
  // The bubble will auto-close.
  explicit NewTabPromo(NewTabButton* new_tab_button, gfx::Rect anchor_rect);
  ~NewTabPromo() override;

 private:
  // BubbleDialogDelegateView:
  int GetDialogButtons() const override;
  void Init() override;
  void OnMouseEntered(const ui::MouseEvent& event) override;
  void OnMouseExited(const ui::MouseEvent& event) override;
  bool OnMousePressed(const ui::MouseEvent& event) override;
  void OnWidgetClosing(views::Widget* widget) override;

  // Closes the New Tab Promo.
  void CloseBubble();

  // Starts a timer which will close the bubble.
  void StartAutoCloseTimer(base::TimeDelta auto_close_duration);

  // Timer used to auto close the bubble.
  base::OneShotTimer timer_;

  // Anchor and owner of the NewTabButton.
  NewTabButton* const new_tab_button_;

  DISALLOW_COPY_AND_ASSIGN(NewTabPromo);
};

#endif  // CHROME_BROWSER_UI_VIEWS_TABS_NEW_TAB_PROMO_H_
