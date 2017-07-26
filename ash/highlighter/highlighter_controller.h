// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_HIGHLIGHTER_HIGHLIGHTER_CONTROLLER_H_
#define ASH_HIGHLIGHTER_HIGHLIGHTER_CONTROLLER_H_

#include <memory>

#include "ash/fast_ink/fast_ink_pointer_controller.h"

namespace ash {

class HighlighterResultView;
class HighlighterSelectionObserver;
class HighlighterView;

// Controller for the highlighter functionality.
// Enables/disables highlighter as well as receives points
// and passes them off to be rendered.
class ASH_EXPORT HighlighterController : public FastInkPointerController {
 public:
  HighlighterController();
  ~HighlighterController() override;

  // Sets the observer. The controller does not own |observer|.
  void SetObserver(HighlighterSelectionObserver* observer);

 private:
  // FastInkPointerController:
  views::View* GetPointerView() const override;
  void CreatePointerView(base::TimeDelta presentation_delay,
                         aura::Window* root_window) override;
  void UpdatePointerView(ui::TouchEvent* event) override;
  void DestroyPointerView() override;

  void DestroyHighlighterView();
  void DestroyResultView();

  // |highlighter_view_| will only hold an instance when the highlighter is
  // enabled and activated (pressed or dragged) and until the fade out
  // animation is done.
  std::unique_ptr<HighlighterView> highlighter_view_;

  // |result_view_| will only hold an instance when the selection result
  // animation is in progress.
  std::unique_ptr<HighlighterResultView> result_view_;

  HighlighterSelectionObserver* observer_;

  DISALLOW_COPY_AND_ASSIGN(HighlighterController);
};

}  // namespace ash

#endif  // ASH_HIGHLIGHTER_HIGHLIGHTER_CONTROLLER_H_
