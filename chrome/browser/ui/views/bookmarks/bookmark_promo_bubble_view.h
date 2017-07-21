// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_BOOKMARKS_BOOKMARK_PROMO_BUBBLE_VIEW_H_
#define CHROME_BROWSER_UI_VIEWS_BOOKMARKS_BOOKMARK_PROMO_BUBBLE_VIEW_H_

#include "base/macros.h"
#include "base/timer/timer.h"
#include "chrome/browser/ui/views/location_bar/location_bar_bubble_delegate_view.h"

namespace ui {
class MouseEvent;
}

// The BookmarkPromoBubbleView is a bubble anchored to the left of the Bookmarks
// button to draw attention to the Bookmarks button. It is called by the
// ToolbarView when prompted by the BookmarkFeatureEngagementTracker.
class BookmarkPromoBubbleView : public LocationBarBubbleDelegateView {
 public:
  // Returns a self owned raw pointer to the ToolbarView.
  static BookmarkPromoBubbleView* CreateSelfOwned(views::View* anchor_view);

 private:
  // Constructs BookmarkPromoBubbleView. Anchors the bubble to the ToolbarView.
  // The bubble widget and promo are owned by their native widget.
  explicit BookmarkPromoBubbleView(views::View* anchor_view);
  ~BookmarkPromoBubbleView() override;

  // LocationBarBubbleDelegateView:
  int GetDialogButtons() const override;
  bool OnMousePressed(const ui::MouseEvent& event) override;
  void OnMouseEntered(const ui::MouseEvent& event) override;
  void OnMouseExited(const ui::MouseEvent& event) override;
  void Init() override;

  // Closes the BookmarkPromoBubbleView.
  void CloseBubble();

  // Starts a timer which will close the bubble.
  void StartAutoCloseTimer(base::TimeDelta auto_close_duration);

  // Timer used to auto close the bubble.
  base::OneShotTimer timer_;

  DISALLOW_COPY_AND_ASSIGN(BookmarkPromoBubbleView);
};

#endif  // CHROME_BROWSER_UI_VIEWS_BOOKMARKS_BOOKMARK_PROMO_BUBBLE_VIEW_H_
