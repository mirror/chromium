// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_EXIT_FULLSCREEN_FAB_INDICATOR_H_
#define CHROME_BROWSER_UI_VIEWS_EXIT_FULLSCREEN_FAB_INDICATOR_H_

#include <memory>

#include "base/macros.h"
#include "ui/gfx/animation/animation_delegate.h"
#include "ui/gfx/animation/slide_animation.h"
#include "ui/gfx/native_widget_types.h"

namespace views {

class Widget;

}  // namespace views

class FabIconView;

////////////////////////////////////////////////////////////////////////////////
//
// ExitFullscreenFabIndicator implements the visual feedback to indicate that
// the user is holding the Escape key and will exit fullscreen after timeout.
// The visual indicator is a FAB with an X icon that drops down from the top of
// the browser window.
//
////////////////////////////////////////////////////////////////////////////////
class ExitFullscreenFabIndicator : private gfx::AnimationDelegate {
 public:
  explicit ExitFullscreenFabIndicator(views::Widget* parent_widget);
  ~ExitFullscreenFabIndicator() override;

  // Shows the FAB indicator with an animation that drops the FAB off the top
  // of |parent_widget|.
  void Show();

  // Hides the FAB indicator. If |animated| is true, the FAB will be hidden by
  // the reversed animation of Show(), i.e. the FAB flies to the top of
  // |parent_widget|.
  void Hide(bool animated);

  bool is_animating() const { return animation_->is_animating(); }

 private:
  // gfx::AnimationDelegate overrides.
  void AnimationProgressed(const gfx::Animation* animation) override;
  void AnimationEnded(const gfx::Animation* animation) override;

  // Animation controlling showing/hiding of the popup.
  std::unique_ptr<gfx::SlideAnimation> animation_;
  views::Widget* parent_widget_;
  FabIconView* fab_view_;
  std::unique_ptr<views::Widget> popup_;

  DISALLOW_COPY_AND_ASSIGN(ExitFullscreenFabIndicator);
};

#endif  // CHROME_BROWSER_UI_VIEWS_EXIT_FULLSCREEN_FAB_INDICATOR_H_
