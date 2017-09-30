// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_LOCK_SCREEN_ACTION_LOCK_SCREEN_ACTION_BACKGROUND_VIEW_H_
#define ASH_SYSTEM_LOCK_SCREEN_ACTION_LOCK_SCREEN_ACTION_BACKGROUND_VIEW_H_

#include "ash/ash_export.h"
#include "base/callback_forward.h"
#include "base/macros.h"
#include "ui/views/animation/ink_drop_observer.h"
#include "ui/views/animation/ink_drop_state.h"
#include "ui/views/widget/widget_delegate.h"

namespace ash {

// Widget delegate view that implements the contents of the lock action
// background widget.
// The widget hosts a view that contains a transparent view with a black ink
// drop. The view implementation provides methods to activate or hide the ink
// drop in the view.
class ASH_EXPORT LockScreenActionBackgroundView
    : public views::WidgetDelegateView,
      public views::InkDropObserver {
 public:
  class ASH_EXPORT TestApi {
   public:
    explicit TestApi(LockScreenActionBackgroundView* action_background_view);
    ~TestApi();

    // Gets the view that paints the background contents.
    views::View* GetBackground();

   private:
    LockScreenActionBackgroundView* action_background_view_;

    DISALLOW_COPY_AND_ASSIGN(TestApi);
  };

  // |on_delete| - callback to be called when the LockScreenActionBackgroundView
  //     gets deleted (e.g. when the widget is closed).
  explicit LockScreenActionBackgroundView(base::OnceClosure on_delete);
  ~LockScreenActionBackgroundView() override;

  // Request the ink drop to be activated.
  // |callback| - called when the ink drop animation ends.
  void AnimateShow(base::OnceClosure callback);

  // Requests the ink drop to be hidden.
  // |callback| - called when the ink drop animation ends.
  void AnimateHide(base::OnceClosure callback);

  // views::InkDropListener:
  void InkDropAnimationStarted() override;
  void InkDropAnimationEnded(views::InkDropState state) override;

  // views::WidgetDelegateView:
  bool CanResize() const override;
  bool CanMaximize() const override;
  bool CanActivate() const override;

 private:
  class NoteBackground;

  base::OnceClosure on_delete_;

  base::OnceClosure animation_end_callback_;
  views::InkDropState animating_to_state_;

  NoteBackground* background_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(LockScreenActionBackgroundView);
};

}  // namespace ash

#endif  // ASH_SYSTEM_LOCK_SCREEN_ACTION_LOCK_SCREEN_ACTION_BACKGROUND_VIEW_H_
