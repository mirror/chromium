// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_WM_SPLITSCREEN_SPLIT_VIEW_CONTROLLER_H_
#define ASH_WM_SPLITSCREEN_SPLIT_VIEW_CONTROLLER_H_

#include "ash/ash_export.h"
#include "ash/wm/overview/overview_window_drag_controller.h"
#include "ash/wm/window_state_observer.h"
#include "ash/wm/wm_types.h"
#include "base/macros.h"
#include "base/observer_list.h"
#include "ui/aura/window_observer.h"

namespace ash {

// The controller for the split view.
class ASH_EXPORT SplitViewController : public aura::WindowObserver,
                                       public ash::wm::WindowStateObserver {
 public:
  enum State { INACTIVE, LEFT_ACTIVE, RIGHT_ACTIVE, BOTH_ACTIVE };

  class Observer {
   public:
    // Called when split view state changed. |window| is a non-null pointer if
    // it just got snapped.
    virtual void OnSplitViewStateChanged(SplitViewController::State state,
                                         aura::Window* window) {}
  };

  SplitViewController();
  ~SplitViewController() override;

  // Returns true if split view mode is active.
  bool IsSplitViewModeActive() const;

  // Snaps window to left/right.
  void SetLeftWindow(aura::Window* left_window);
  void SetRightWindow(aura::Window* right_window);

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  // aura::WindowObserver overrides:
  void OnWindowDestroying(aura::Window* window) override;

  // ash::wm::WindowStateObserver overrides:
  void OnPostWindowStateTypeChange(ash::wm::WindowState* window_state,
                                   ash::wm::WindowStateType old_type) override;

  aura::Window* left_window() { return left_window_; }
  aura::Window* right_window() { return right_window_; }
  State state() { return state_; }

 private:
  // Ends the split view mode and does necessary cleanup.
  void EndSplitView(bool animate);

  // Stops observing |window| and restore its original bounds.
  void StopObservingAndRestore(aura::Window* window,
                               const gfx::Rect original_bounds,
                               bool animate);

  // Notifies observers that split view has been changed because of |window|.
  // 1. INACTIVE ---> LEFT_ACTIVE (|window| was snapped to left)
  // 2. INACTIVE ---> RIGHT_ACTIVE (|window| was snapped to right)
  // 3. LEFT_ACTIVE ---> LEFT_ACTIVE (|window| was snapped to left)
  // 4. LEFT_ACTIVE ---> BOTH_ACTIVE (|window| was snapped to right)
  // 5. LEFT_ACTIVE ---> INACTIVE (|window| was closed)
  // 6. RIGHT_ACTIVE ---> RIGHT_ACTIVE (|window| was snapped to right)
  // 7. RIGHT_ACTIVE ---> BOTH_ACTIVE (|window| was snapped to left)
  // 8. RIGHT_ACTIVE ---> INACTIVE (|window| was closed)
  // 9. BOTH_ACTIVE ---> LEFT_ACTIVE (|window| was closed)
  // 10. BOTH_ACTIVE ---> RIGHT_ACTIVE (|window| was closed)
  void NotifySplitViewStateChanged(State previous_state,
                                   State current_state,
                                   aura::Window* window);

  aura::Window* left_window_ = nullptr;
  aura::Window* right_window_ = nullptr;

  gfx::Rect left_window_original_bounds_;
  gfx::Rect right_window_original_bounds_;

  // Current snap state.
  State state_ = INACTIVE;

  // Previous snap state.
  State previous_state_ = INACTIVE;

  base::ObserverList<Observer> observers_;

  DISALLOW_COPY_AND_ASSIGN(SplitViewController);
};

}  // namespace ash

#endif  // ASH_WM_SPLITSCREEN_SPLIT_VIEW_CONTROLLER_H_
