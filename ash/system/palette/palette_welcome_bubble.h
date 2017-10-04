// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_PALETTE_PALETTE_WELCOME_BUBBLE_H_
#define ASH_SYSTEM_PALETTE_PALETTE_WELCOME_BUBBLE_H_

#include "ash/ash_export.h"
#include "base/macros.h"
#include "ui/views/widget/widget_observer.h"

namespace views {
class BubbleDialogDelegateView;
class ImageButton;
class Widget;
}  // namespace views

namespace ash {
class PaletteTray;

// The PaletteWelcomeBubble handles displaying a warm welcome bubble letting
// users know about the PaletteTray the first time a stylus is ejected, or if an
// external stylus is detected.
class ASH_EXPORT PaletteWelcomeBubble : public views::WidgetObserver {
 public:
  explicit PaletteWelcomeBubble(PaletteTray* tray);
  ~PaletteWelcomeBubble() override;

  // views::WidgetObservers:
  void OnWidgetClosing(views::Widget* widget) override;
  void OnWidgetDestroying(views::Widget* widget) override;

  void Show();
  void Hide();

  bool BubbleShown() { return bubble_view_ != nullptr; }

  // Returns the close button on the bubble if it exists for testing purposes.
  views::ImageButton* GetCloseButtonForTest();

 private:
  friend class PaletteWelcomeBubbleTest;
  class WelcomeBubbleView;

  // The PaletteTray this bubble is associated with. Serves as the anchor for
  // the bubble. Not owned.
  PaletteTray* tray_ = nullptr;

  views::BubbleDialogDelegateView* bubble_view_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(PaletteWelcomeBubble);
};

}  // namespace ash

#endif  // ASH_SYSTEM_PALETTE_PALETTE_WELCOME_BUBBLE_H_
