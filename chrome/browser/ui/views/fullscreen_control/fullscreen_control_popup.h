// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_FULLSCREEN_CONTROL_FULLSCREEN_CONTROL_POPUP_H_
#define CHROME_BROWSER_UI_VIEWS_FULLSCREEN_CONTROL_FULLSCREEN_CONTROL_POPUP_H_

#include <memory>

#include "base/callback_forward.h"
#include "base/macros.h"
#include "ui/gfx/animation/animation_delegate.h"
#include "ui/gfx/animation/slide_animation.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/native_widget_types.h"

namespace views {
class Widget;
}  // namespace views

class FullscreenControlView;

// FullscreenControlPopup is a helper class that holds a FullscreenControlView
// and allow showing and hiding the view with a drop down animation.
//
// TODO(yuweih): We might be able to use the DropdownBarHost for the animation
// once Mac starts using views. In that case we will need to change the
// animation curve and hook the opacity animation into DropdownBarHost.
class FullscreenControlPopup : public gfx::AnimationDelegate {
 public:
  FullscreenControlPopup(gfx::NativeView parent_view,
                         const base::RepeatingClosure& on_button_pressed);
  ~FullscreenControlPopup() override;

  // Shows the indicator with an animation that drops it off the top of
  // |parent_view|.
  // parent_bounds_in_screen: The bounds of |parent_view| in the reference frame
  // of the screen.
  void Show(const gfx::Rect& parent_bounds_in_screen);

  // Hides the indicator. If |animated| is true, the indicator will be hidden by
  // the reversed animation of Show(), i.e. the indicator flies to the top of
  // |parent_widget|.
  void Hide(bool animated);

 private:
  // gfx::AnimationDelegate:
  void AnimationProgressed(const gfx::Animation* animation) override;
  void AnimationEnded(const gfx::Animation* animation) override;

  FullscreenControlView* const control_view_;
  std::unique_ptr<views::Widget> const popup_;
  std::unique_ptr<gfx::SlideAnimation> const animation_;

  gfx::Rect parent_bounds_in_screen_;

  DISALLOW_COPY_AND_ASSIGN(FullscreenControlPopup);
};

#endif  // CHROME_BROWSER_UI_VIEWS_FULLSCREEN_CONTROL_FULLSCREEN_CONTROL_POPUP_H_
