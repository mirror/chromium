// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_HIGHLIGHTER_HIGHLIGHTER_CONTROLLER_H_
#define ASH_HIGHLIGHTER_HIGHLIGHTER_CONTROLLER_H_

#include <memory>

#include "ash/fast_ink/fast_ink_pointer_controller.h"
#include "ash/public/interfaces/highlighter_controller.mojom.h"
#include "base/callback.h"
#include "mojo/public/cpp/bindings/binding_set.h"

namespace base {
class OneShotTimer;
}

namespace ash {

class HighlighterResultView;
class HighlighterView;

// Controller for the highlighter functionality.
// Enables/disables highlighter as well as receives points
// and passes them off to be rendered.
class ASH_EXPORT HighlighterController : public FastInkPointerController,
                                         public mojom::HighlighterController {
 public:
  HighlighterController();
  ~HighlighterController() override;

  // Set the callback to exit the highlighter mode. If |allow_retries| is true,
  // the callback will be called after a successful gesture recognition. If
  // |allow_retries| is false, the callback will be  called after the first
  // complete gesture, regardless of the recognition result.
  void SetExitCallback(base::OnceClosure callback, bool allow_retries);

  // FastInkPointerController:
  void SetEnabled(bool enabled) override;

  void BindRequest(mojom::HighlighterControllerRequest request);

  // mojom::HighlighterController:
  void SetClient(mojom::HighlighterControllerClientPtr client) override;

 private:
  friend class HighlighterControllerTestApi;

  // FastInkPointerController:
  views::View* GetPointerView() const override;
  void CreatePointerView(base::TimeDelta presentation_delay,
                         aura::Window* root_window) override;
  void UpdatePointerView(ui::TouchEvent* event) override;
  void DestroyPointerView() override;
  bool CanStartNewGesture(ui::TouchEvent* event) override;

  // Performs gesture recognition, initiates appropriate visual effects,
  // notifies the observer if necessary.
  void RecognizeGesture();

  // Destroys |highlighter_view_|, if it exists.
  void DestroyHighlighterView();

  // Destroys |result_view_|, if it exists.
  void DestroyResultView();

  // Calls and clears the mode exit callback, if it is set.
  void CallExitCallback();

  void FlushMojoForTesting();

  // |highlighter_view_| will only hold an instance when the highlighter is
  // enabled and activated (pressed or dragged) and until the fade out
  // animation is done.
  std::unique_ptr<HighlighterView> highlighter_view_;

  // |result_view_| will only hold an instance when the selection result
  // animation is in progress.
  std::unique_ptr<HighlighterResultView> result_view_;

  // Time of the session start (e.g. when the controller was enabled).
  base::TimeTicks session_start_;

  // Time of the previous gesture end, valid after the first gesture
  // within the session is complete.
  base::TimeTicks previous_gesture_end_;

  // Gesture counter withing a session.
  int gesture_counter_ = 0;

  // Recognized gesture counter withing a session.
  int recognized_gesture_counter_ = 0;

  // Not null while waiting for the next event to continue an interrupted
  // stroke.
  std::unique_ptr<base::OneShotTimer> interrupted_stroke_timer_;

  // The callback to exit the mode in the UI.
  base::OnceClosure exit_callback_;

  // If true, the mode is not exited until a valid selection is made.
  bool allow_retries_ = true;

  // Bindings for mojom::HighlighterController interface.
  mojo::BindingSet<mojom::HighlighterController> bindings_;
  // Interface to highlighter controller client (chrome).
  mojom::HighlighterControllerClientPtr client_;

  DISALLOW_COPY_AND_ASSIGN(HighlighterController);
};

}  // namespace ash

#endif  // ASH_HIGHLIGHTER_HIGHLIGHTER_CONTROLLER_H_
