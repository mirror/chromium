// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SHELF_APP_LIST_BACK_BUTTON_BACKGROUND_VIEW_H_
#define ASH_SHELF_APP_LIST_BACK_BUTTON_BACKGROUND_VIEW_H_

#include <memory>

#include "ash/ash_export.h"
#include "ash/session/session_observer.h"
#include "ash/shell_observer.h"
#include "ash/voice_interaction/voice_interaction_observer.h"
#include "base/macros.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/views/controls/button/image_button.h"

namespace ash {
class AppListButton;
class BackButton;
class InkDropButtonListener;
class Shelf;
class ShelfView;

// Button used for the AppList icon on the shelf.
class ASH_EXPORT AppListBackButtonBackgroundView : public views::View {
 public:
  static const char kViewClassName[];

  AppListBackButtonBackgroundView(InkDropButtonListener* listener,
                                  ShelfView* shelf_view,
                                  Shelf* shelf);
  ~AppListBackButtonBackgroundView() override;

  // Updates background and schedules a paint.
  void UpdateShelfItemBackground(SkColor color);

  // Called by ShelfView to notify the app list button that it has started or
  // finished a bounds animation.
  void OnBoundsAnimationStarted();
  void OnBoundsAnimationFinished();

  AppListButton* GetAppListButton() { return app_list_button_; }

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

  ShelfView* shelf_view_ = nullptr;
  Shelf* shelf_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(AppListBackButtonBackgroundView);
};

}  // namespace ash

#endif  // ASH_SHELF_APP_LIST_BACK_BUTTON_BACKGROUND_VIEW_H_
