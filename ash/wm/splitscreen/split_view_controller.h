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

  // Returns true if a window can be snapped to |snap_type|. For simplicity, we
  // don't allow more than one window snapped at the same position.
  bool CanSnap(OverviewWindowDragController::SnapType snap_type) const;
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

  // Notifies observers that the split view state has been changed because of
  // |window|. There are only 4 kinds of state change, see below:
  // 1. INACTIVE ---> LEFT_ACTIVE / RIGHT_ACTIVE (|window| was snapped)
  // 2. LEFT_ACTIVE / RIGHT_ACTIVE ---> BOTH_ACTIVE (|window| was snapped)
  // 2. LEFT_ACTIVE / RIGHT_ACTIVE ---> INACTIVE (|window| was closed)
  // 3. BOTH_ACTIVE ---> INACTIVE (|window| was closed)
  void NotifySplitViewStateChanged(State state, aura::Window* window);

  aura::Window* left_window_ = nullptr;
  aura::Window* right_window_ = nullptr;

  gfx::Rect left_window_original_bounds_;
  gfx::Rect right_window_original_bounds_;

  // Current snap state.
  State state_ = INACTIVE;

  base::ObserverList<Observer> observers_;

  DISALLOW_COPY_AND_ASSIGN(SplitViewController);
};

}  // namespace ash

#endif  // ASH_WM_SPLITSCREEN_SPLIT_VIEW_CONTROLLER_H_
