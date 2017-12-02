// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SHELF_APP_LIST_BACK_BUTTON_BACKGROUND_VIEW_H_
#define ASH_SHELF_APP_LIST_BACK_BUTTON_BACKGROUND_VIEW_H_

#include "ash/ash_export.h"
#include "base/macros.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/views/view.h"

namespace ash {
class AppListButton;
class BackButton;
class InkDropButtonListener;
class Shelf;
class ShelfView;

// The view that handles drawing the background of the launcher button and the
// background of the back button in tablet mode as well. In laptop mode, the
// back button is not shown, so this class draws a circle background behind the
// launcher button. In tablet mode it draws a background consisting of two
// circles behind each button joined by a rectangle. It will look something like
// below where 1. is the back button and 2. is the app launcher button.
//
//     ----------------------
//    /                      \
//    |  1.               2. |
//    \______________________/
//
class ASH_EXPORT AppListBackButtonBackgroundView : public views::View {
 public:
  static const char kViewClassName[];

  AppListBackButtonBackgroundView(InkDropButtonListener* listener,
                                  ShelfView* shelf_view,
                                  Shelf* shelf);
  ~AppListBackButtonBackgroundView() override;

  // Updates background and schedules a paint.
  void UpdateShelfItemBackground(SkColor color);

  // Called by ShelfView to notify that it has finished a bounds animation.
  void OnBoundsAnimationFinished();

  AppListButton* app_list_button() { return app_list_button_; }
  BackButton* back_button() { return back_button_; }

 protected:
  // views::View:
  const char* GetClassName() const override;
  void OnPaint(gfx::Canvas* canvas) override;
  void Layout() override;

 private:
  AppListButton* app_list_button_ = nullptr;
  BackButton* back_button_ = nullptr;

  // Color used to paint the background.
  SkColor background_color_;

  Shelf* shelf_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(AppListBackButtonBackgroundView);
};

}  // namespace ash

#endif  // ASH_SHELF_APP_LIST_BACK_BUTTON_BACKGROUND_VIEW_H_
