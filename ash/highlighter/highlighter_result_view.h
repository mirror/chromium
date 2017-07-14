// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_HIGHLIGHTER_HIGHLIGHTER_RESULT_VIEW_H_
#define ASH_HIGHLIGHTER_HIGHLIGHTER_RESULT_VIEW_H_

#include <memory>

#include "base/macros.h"
#include "ui/views/view.h"

namespace aura {
class Window;
}

namespace base {
class Timer;
}

namespace ui {
class Layer;
}

namespace views {
class Widget;
}

namespace ash {

class HighlighterResultView : public views::View {
 public:
  HighlighterResultView(aura::Window* root_window);

  void AnimateInPlace(const gfx::Rect& bounds, SkColor color);
  void AnimateDeflate(const gfx::Rect& bounds);

 private:
  void ScheduleFadeIn(long delay_ms, long duration_ms);
  void FadeIn(long duration_ms);
  void FadeOut();

  std::unique_ptr<views::Widget> widget_;
  std::unique_ptr<ui::Layer> result_layer_;
  std::unique_ptr<base::Timer> animation_timer_;

  DISALLOW_COPY_AND_ASSIGN(HighlighterResultView);
};

}  // namespace ash

#endif  // ASH_HIGHLIGHTER_HIGHLIGHTER_RESULT_VIEW_H_
