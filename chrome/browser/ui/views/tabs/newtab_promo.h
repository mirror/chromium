// Copyright 2017 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_TABS_NEWTAB_PROMO_H_
#define CHROME_BROWSER_UI_VIEWS_TABS_NEWTAB_PROMO_H_

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

namespace content {
class WebContents;
}

class NewTabButton;

// View used to display Newtab promo when it has been indicated that the user
// needs to see it.
class NewTabPromo : public views::BubbleDialogDelegateView {
 public:
  // Shows the bubble and automatically closes it after a short time period.
  static void ShowBubble(NewTabButton* newtab_button);

  // Returns true when the promo is showing.
  static bool ShowingPromo();

  // Closes the showing bubble (if one exists).
  static void CloseCurrentBubble();

  // Returns the zoom bubble if the zoom bubble is showing. Returns NULL
  // otherwise.
  static NewTabPromo* GetNewTabPromo();

 protected:
  // The class listens for WebContentsView events and closes the bubble. Useful
  // for bubbles that do not start out focused but need to close when the user
  // interacts with the web view.
  class WebContentMouseHandler : public ui::EventHandler {
   public:
    WebContentMouseHandler(NewTabPromo* bubble,
                           content::WebContents* web_contents);
    ~WebContentMouseHandler() override;

    void OnMouseEvent(ui::MouseEvent* event) override;
    void OnTouchEvent(ui::TouchEvent* event) override;

   private:
    NewTabPromo* bubble_;
    content::WebContents* web_contents_;
    std::unique_ptr<views::EventMonitor> event_monitor_;

    DISALLOW_COPY_AND_ASSIGN(WebContentMouseHandler);
  };

 private:
  // Constructs NewTabPromo. Anchors the bubble to the NewTabButton.
  // The bubble will auto-close.
  explicit NewTabPromo(NewTabButton* newtab_button);
  ~NewTabPromo() override;

  // BubbleDelegateView:
  int GetDialogButtons() const override;
  void OnMouseEntered(const ui::MouseEvent& event);
  void OnMouseExited(const ui::MouseEvent& event);
  void OnMouseReleased(const ui::MouseEvent& event);
  void Init() override;

  // Closes the bubble.
  void CloseBubble();

  // Starts a timer which will close the bubble
  void StartTimer();

  // Stops the auto-close timer
  void StopTimer();

  // Label will hold the text to be displayed.
  views::Label* label_;

  // Timer used to auto close the bubble.
  base::OneShotTimer timer_;

  // Timer duration that is made shorter if a user hovers over the button.
  base::TimeDelta auto_close_duration_;

  // The Newtab Button that the promo is anchored to.
  NewTabButton* newtab_button_;

  // Singleton instance of the newtab promo bubble. The promo bubble can only be
  // shown on the active browser window, so there is no case in which it will be
  // shown twice at the same time.
  static NewTabPromo* newtab_promo_;

  std::unique_ptr<WebContentMouseHandler> mouse_handler_;
};

#endif  // CHROME_BROWSER_UI_VIEWS_TABS_NEWTAB_PROMO_H_
