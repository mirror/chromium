// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_TABS_NEW_TAB_PROMO_H_
#define CHROME_BROWSER_UI_VIEWS_TABS_NEW_TAB_PROMO_H_

#include "base/macros.h"
#include "base/timer/timer.h"
#include "ui/views/bubble/bubble_dialog_delegate.h"

namespace gfx {
class Rect;
}

namespace ui {
class MouseEvent;
}

// The NewTabPromo is a bubble anchored to the right of the new tab button. It
// draws users attention to the newtab button. It is called by the NewTabButton
// when prompted by the NewTabFeatureEngagementTracker.
class NewTabPromo : public views::BubbleDialogDelegateView {
 public:
  // Returns a self owned raw pointer to the NewTabButton. |promo_string| is
  // the text that the promo will show.
  static NewTabPromo* CreateSelfOwned(
      const gfx::Rect& anchor_rect,
      const std::string& promo_string_specifier);

  // Returns a bool based off of the visibility of the NewTabButton's widget.
  bool IsPromoVisible();

 private:
  // Constructs NewTabPromo. Anchors the bubble to the NewTabButton.
  // The bubble widget and promo are owned by their native widget.

  NewTabPromo(const gfx::Rect& anchor_rect,
              const std::string& promo_string_specifier);
  ~NewTabPromo() override;

  // BubbleDialogDelegateView:
  int GetDialogButtons() const override;
  bool OnMousePressed(const ui::MouseEvent& event) override;
  void OnMouseEntered(const ui::MouseEvent& event) override;
  void OnMouseExited(const ui::MouseEvent& event) override;
  void Init() override;

  // Closes the New Tab Promo.
  void CloseBubble();

  // Starts a timer which will close the bubble.
  void StartAutoCloseTimer(base::TimeDelta auto_close_duration);

  // Based off of the specifier string passed through the constuctor, generates
  // the appropriate string to be displayed in the NewTabPromo.
  void SetStringForPromo(const std::string& specifier);

  // Timer used to auto close the bubble.
  base::OneShotTimer timer_;

  // Text shown on promotional UI appearing next to the New Tab button.
  base::string16 promo_string_;

  DISALLOW_COPY_AND_ASSIGN(NewTabPromo);
};

#endif  // CHROME_BROWSER_UI_VIEWS_TABS_NEW_TAB_PROMO_H_
