// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_BOOKMARKS_BOOKMARK_PROMO_BUBBLE_VIEW_H_
#define CHROME_BROWSER_UI_VIEWS_BOOKMARKS_BOOKMARK_PROMO_BUBBLE_VIEW_H_

#include "base/macros.h"
#include "base/scoped_observer.h"
#include "base/timer/timer.h"
#include "chrome/browser/ui/views/location_bar/location_bar_bubble_delegate_view.h"
#include "ui/views/widget/widget_observer.h"

namespace bookmarks {
class BookmarkBubbleObserver;
}

namespace ui {
class MouseEvent;
}

// The BookmarkPromoBubbleView is a bubble anchored to the left of the Bookmarks
// button to draw attention to the Bookmarks button. It is called by the
// ToolbarView when prompted by the BookmarkFeatureEngagementTracker.
class BookmarkPromoBubbleView : public LocationBarBubbleDelegateView {
 public:
  static views::Widget* ShowBubble(views::View* anchor_view,
                                   bookmarks::BookmarkBubbleObserver* observer);

  static bool IsObservingSources();

  static BookmarkPromoBubbleView* bookmark_promo_bubble() {
    return bookmark_promo_bubble_;
  }

 private:
  // Constructs BookmarkPromoBubbleView. Anchors the bubble to |anchor_view|.
  // The bubble widget and promo are owned by their native widget.
  explicit BookmarkPromoBubbleView(views::View* anchor_view,
                                   bookmarks::BookmarkBubbleObserver* observer);
  ~BookmarkPromoBubbleView() override;

  // LocationBarBubbleDelegateView:
  int GetDialogButtons() const override;
  bool OnMousePressed(const ui::MouseEvent& event) override;
  void OnMouseEntered(const ui::MouseEvent& event) override;
  void OnMouseExited(const ui::MouseEvent& event) override;

  // views::WidgetObserver:
  void OnWidgetDestroying(views::Widget* widget) override;

  // Closes the BookmarkPromoBubbleView.
  void CloseBubble();

  // Starts a timer which will close the bubble.
  void StartAutoCloseTimer(base::TimeDelta auto_close_duration);

  // Timer used to auto close the bubble.
  base::OneShotTimer timer_;

  // Our observer, to notify when the bubble shows or hides.
  bookmarks::BookmarkBubbleObserver* observer_;

  // The bookmark promo bubble, if we're showing one.
  static BookmarkPromoBubbleView* bookmark_promo_bubble_;

  // Observes the BookmarkPromo's widget. Used to tell whether the promo is
  // open and gets called back when it closes.
  ScopedObserver<views::Widget, WidgetObserver> bookmark_promo_observer_;

  DISALLOW_COPY_AND_ASSIGN(BookmarkPromoBubbleView);
};

#endif  // CHROME_BROWSER_UI_VIEWS_BOOKMARKS_BOOKMARK_PROMO_BUBBLE_VIEW_H_
