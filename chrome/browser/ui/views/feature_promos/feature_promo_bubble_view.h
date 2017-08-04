// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_FEATURE_PROMOS_FEATURE_PROMO_BUBBLE_VIEW_H_
#define CHROME_BROWSER_UI_VIEWS_FEATURE_PROMOS_FEATURE_PROMO_BUBBLE_VIEW_H_

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

 protected:
  // |anchor_view| will be primarily used to anchor the FeaturePromoBubbleView,
  // the |anchor_rect| will be used alternatively for bubble views that have
  // custom positioning requirements. To use the |anchor_rect|, pass in a
  // nullptr for |anchor_view|. The |anchor_view| is always preferred, so if it
  // exists, it will be used over the |anchor_rect|. The |arrow| sets where the
  // arrow hangs off the bubble and the |string_specifier| is the text that is
  // displayed on the bubble.
  FeaturePromoBubbleView(views::View* anchor_view,
                         const gfx::Rect& anchor_rect,
                         views::BubbleBorder::Arrow arrow,
                         int string_specifier);

 private:
  // BubbleDialogDelegateView:
  int GetDialogButtons() const override;
  bool OnMousePressed(const ui::MouseEvent& event) override;
  void OnMouseEntered(const ui::MouseEvent& event) override;
  void OnMouseExited(const ui::MouseEvent& event) override;

  // Closes the promo bubble.
  void CloseBubble();

  // Starts a timer to close the promo bubble.
  void StartAutoCloseTimer(base::TimeDelta auto_close_duration);

  // Timer used to auto close the bubble.
  base::OneShotTimer timer_;

  DISALLOW_COPY_AND_ASSIGN(FeaturePromoBubbleView);
};

#endif  // CHROME_BROWSER_UI_VIEWS_FEATURE_PROMOS_FEATURE_PROMO_BUBBLE_VIEW_H_
