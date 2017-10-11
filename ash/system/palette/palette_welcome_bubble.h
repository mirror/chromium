// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_PALETTE_PALETTE_WELCOME_BUBBLE_H_
#define ASH_SYSTEM_PALETTE_PALETTE_WELCOME_BUBBLE_H_

#include "ash/ash_export.h"
#include "ash/shell_observer.h"
#include "base/macros.h"
#include "ui/views/pointer_watcher.h"
#include "ui/views/widget/widget_observer.h"

class PrefRegistrySimple;
class PrefService;

namespace views {
class ImageButton;
class Widget;
}  // namespace views

namespace ash {
class PaletteTray;

// The PaletteWelcomeBubble handles displaying a warm welcome bubble letting
// users know about the PaletteTray the first time a stylus is ejected, or if an
// external stylus is detected.
class ASH_EXPORT PaletteWelcomeBubble : public ShellObserver,
                                        public views::WidgetObserver,
                                        public views::PointerWatcher {
 public:
  explicit PaletteWelcomeBubble(PaletteTray* tray);
  ~PaletteWelcomeBubble() override;

  static void RegisterLocalStatePrefs(PrefRegistrySimple* registry);

  // ShellObserver:
  void OnLocalStatePrefServiceInitialized(PrefService* pref_service) override;

  // views::WidgetObserver:
  void OnWidgetClosing(views::Widget* widget) override;
  void OnWidgetDestroying(views::Widget* widget) override;

  // views::PointerWatcher:
  void OnPointerEventObserved(const ui::PointerEvent& event,
                              const gfx::Point& location_in_screen,
                              gfx::NativeView target) override;

  void ShowIfNeeded();

  bool BubbleShown() { return bubble_view_ != nullptr; }

  // Returns the close button on the bubble if it exists.
  views::ImageButton* GetCloseButtonForTest();

  // Returns the bounds of the bubble view if it exists.
  gfx::Rect GetBubbleBoundsForTest();

 private:
  friend class PaletteWelcomeBubbleTest;
  class WelcomeBubbleView;

  void Show();
  void Hide();

  // The PaletteTray this bubble is associated with. Serves as the anchor for
  // the bubble. Not owned.
  PaletteTray* tray_ = nullptr;

  PrefService* local_state_pref_service_ = nullptr;  // Not owned.

  WelcomeBubbleView* bubble_view_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(PaletteWelcomeBubble);
};

}  // namespace ash

#endif  // ASH_SYSTEM_PALETTE_PALETTE_WELCOME_BUBBLE_H_
