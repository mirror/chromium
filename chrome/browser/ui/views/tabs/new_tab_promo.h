// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_TABS_NEW_TAB_PROMO_H_
#define CHROME_BROWSER_UI_VIEWS_TABS_NEW_TAB_PROMO_H_

#include "chrome/browser/ui/views/tabs/tab_strip.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/text_utils.h"
#include "ui/views/bubble/bubble_dialog_delegate.h"
#include "ui/views/controls/label.h"
#include "ui/views/event_monitor.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/layout/fill_layout.h"

class TabStrip;
class NewTabButton;

// View used to display Newtab promo when it has been indicated that the user
// needs to see it.
class NewTabPromo : public views::BubbleDialogDelegateView {
 public:
  // Closes the bubble.
  void CloseBubble();

  // Constructs NewTabPromo. Anchors the bubble to the NewTabButton.
  // The bubble will auto-close.
  explicit NewTabPromo(TabStrip* tab_strip_);
  ~NewTabPromo() override;

  // Notifies the Tabstip that the promo has closed
  void NewTabPromo::NotifyTabStripOnClose();

 private:
  // View:
  int GetDialogButtons() const override;
  void OnMouseEntered(const ui::MouseEvent& event) override;
  void OnMouseExited(const ui::MouseEvent& event) override;
  bool OnMousePressed(const ui::MouseEvent& event) override;

  void Init() override;

  // Starts a timer which will close the bubble
  void StartTimer();

  // Stops the auto-close timer
  void StopTimer();

  // Timer used to auto close the bubble.
  base::OneShotTimer timer_;

  // Timer duration that is made shorter if a user hovers over the button.
  base::TimeDelta auto_close_duration_;

  // The Newtab Button that the promo is anchored to. Owner of this NewTabPromo.
  TabStrip* tab_strip_;
};

#endif  // CHROME_BROWSER_UI_VIEWS_TABS_NEW_TAB_PROMO_H_
