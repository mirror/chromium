// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_HIGHLIGHTER_HIGHLIGHTER_VIEW_H_
#define ASH_HIGHLIGHTER_HIGHLIGHTER_VIEW_H_

#include <vector>

#include "ash/fast_ink/fast_ink_points.h"
#include "ash/fast_ink/fast_ink_view.h"
#include "base/time/time.h"

namespace aura {
class Window;
}

namespace base {
class Timer;
}

namespace ash {

// HighlighterView displays the highlighter palette tool. It draws the
// highlighter stroke which consists of a series of thick lines connecting
// touch points.
class HighlighterView : public FastInkView {
 public:
  enum class AnimationMode {
    kFadeout,
    kInflate,
    kDeflate,
  };

  static const SkColor kPenColor;
  static const gfx::SizeF kPenTipSize;

  HighlighterView(base::TimeDelta presentation_delay,
                  aura::Window* root_window);
  ~HighlighterView() override;

  gfx::Rect GetBoundingBox() const;

  std::vector<gfx::PointF> GetTrace() const;

  void AddNewPoint(const gfx::PointF& point, const base::TimeTicks& time);

  void Animate(const gfx::Point& pivot,
               AnimationMode animation_mode,
               const base::Closure& done);

 private:
  friend class HighlighterControllerTestApi;

  // FastInkView:
  void OnRedraw(gfx::Canvas& canvas, const gfx::Vector2d& offset) override;

  void FadeOut(const gfx::Point& pivot,
               AnimationMode animation_mode,
               const base::Closure& done);

  FastInkPoints points_;
  FastInkPoints predicted_points_;
  const base::TimeDelta presentation_delay_;

  std::unique_ptr<base::Timer> animation_timer_;

  DISALLOW_COPY_AND_ASSIGN(HighlighterView);
};

}  // namespace ash

#endif  // ASH_HIGHLIGHTER_HIGHLIGHTER_VIEW_H_
