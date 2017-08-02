// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_FEATURE_PROMOS_FEATURE_PROMO_H_
#define CHROME_BROWSER_UI_VIEWS_FEATURE_PROMOS_FEATURE_PROMO_H_

#include "base/macros.h"
#include "base/timer/timer.h"
#include "ui/views/bubble/bubble_dialog_delegate.h"

namespace gfx {
class Rect;
}

namespace ui {
class MouseEvent;
}

// The FeaturePromoBubbleView is a special BubbleDialogDelegateView for
// in-product help which educates users about certain Chrome features in a
// deferred context.
class FeaturePromoBubbleView : public views::BubbleDialogDelegateView {
 public:
  ~FeaturePromoBubbleView() override;

  // Returns a raw pointer that is owned by its native widget.
  static FeaturePromoBubbleView* Create(views::View* anchor_view,
                                        const gfx::Rect& anchor_rect);

  // Returns the amount of time the promo should stay onscreen if the user
  // never hovers over it.
  base::TimeDelta DelayDefault();

  // Returns the amount of time the promo should stay onscreen after the
  // user stops hovering over it.
  base::TimeDelta DelayShort();

 protected:
  explicit FeaturePromoBubbleView(views::View* anchor_view,
                                  const gfx::Rect& anchor_rect,
                                  views::BubbleBorder::Arrow arrow);

 private:
  // BubbleDialogDelegateView:
  int GetDialogButtons() const override;
  bool OnMousePressed(const ui::MouseEvent& event) override;
  void OnMouseEntered(const ui::MouseEvent& event) override;
  void OnMouseExited(const ui::MouseEvent& event) override;

  // Closes the promo bubble.
  void CloseBubble();

  // Gets the string that is displayed in the promo that is retrieved from
  // Finch.
  virtual int GetStringSpecifier();

  // Starts a timer to close the promo bubble.
  void StartAutoCloseTimer(base::TimeDelta auto_close_duration);

  // Timer used to auto close the bubble.
  base::OneShotTimer timer_;

  DISALLOW_COPY_AND_ASSIGN(FeaturePromoBubbleView);
};

#endif  // CHROME_BROWSER_UI_VIEWS_FEATURE_PROMOS_FEATURE_PROMO_H_
