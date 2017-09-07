// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_EXIT_FULLSCREEN_INDICATOR_H_
#define CHROME_BROWSER_UI_VIEWS_EXIT_FULLSCREEN_INDICATOR_H_

#include <memory>

#include "base/macros.h"
#include "ui/compositor/layer_animation_observer.h"
#include "ui/gfx/geometry/rect.h"

namespace views {

class Widget;

}  // namespace views

class ExitFullscreenIndicatorView;

////////////////////////////////////////////////////////////////////////////////
//
// ExitFullscreenIndicator implements the visual feedback to indicate that
// the user is holding the Escape key and will exit fullscreen after timeout.
// The visual indicator is a partially-transparent black circle with a "X" icon
// that drops down from the top of the browser window.
//
////////////////////////////////////////////////////////////////////////////////
class ExitFullscreenIndicator : private ui::ImplicitAnimationObserver {
 public:
  explicit ExitFullscreenIndicator(views::Widget* parent_widget);
  ~ExitFullscreenIndicator() override;

  // Shows the indicator with an animation that drops it off the top of
  // |parent_widget|.
  void Show();

  // Hides the indicator. If |animated| is true, the indicator will be hidden by
  // the reversed animation of Show(), i.e. the indicator flies to the top of
  // |parent_widget|.
  void Hide(bool animated);

 private:
  // ui::ImplicitAnimationObserver overrides.
  void OnImplicitAnimationsCompleted() override;

  gfx::Rect CalculateBounds(int y_offset) const;

  views::Widget* parent_widget_;
  ExitFullscreenIndicatorView* indicator_view_;
  std::unique_ptr<views::Widget> popup_;

  DISALLOW_COPY_AND_ASSIGN(ExitFullscreenIndicator);
};

#endif  // CHROME_BROWSER_UI_VIEWS_EXIT_FULLSCREEN_INDICATOR_H_
