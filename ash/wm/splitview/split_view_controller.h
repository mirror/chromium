// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_WM_SPLITSVIEW_SPLIT_VIEW_CONTROLLER_H_
#define ASH_WM_SPLITSVIEW_SPLIT_VIEW_CONTROLLER_H_

#include "ash/ash_export.h"
#include "ash/shell_observer.h"
#include "ash/wm/window_state_observer.h"
#include "base/macros.h"
#include "base/observer_list.h"
#include "ui/aura/window_observer.h"
#include "ui/wm/public/activation_change_observer.h"

namespace ash {

// The controller for the split view. It snaps a window to left/right side of
// the screen. It also observes the two snapped windows and decides when to exit
// the split view mode.
class ASH_EXPORT SplitViewController : public aura::WindowObserver,
                                       public ash::wm::WindowStateObserver,
                                       public ::wm::ActivationChangeObserver,
                                       public ShellObserver {
 public:
  enum State { NOSNAP, LEFT_SNAPPED, RIGHT_SNAPPED, BOTH_SNAPPED };

  class Observer {
   public:
    // Called when split view state changed from |previous_state| to |state|.
    virtual void OnSplitViewStateChanged(
        SplitViewController::State previous_state,
        SplitViewController::State state) {}
  };

  SplitViewController();
  ~SplitViewController() override;

  // Returns true if split view mode is supported. Currently the split view
  // mode is only supported in maximized mode (tablet mode).
  static bool ShouldAllowSplitView();

  // Returns true if split view mode is active.
  bool IsSplitViewModeActive() const;

  // Snaps window to left/right.
  void SetLeftWindow(aura::Window* left_window);
  void SetRightWindow(aura::Window* right_window);

  // Returns the default snapped window. It's the window that remains open until
  // the split mode ends. It's decided by |default_snap_position_|. E.g., If
  // |default_snap_position_| equals LEFT, then the default snapped window is
  // |left_window_|. All the other window will open on the right side.
  aura::Window* GetDefaultSnappedWindow();

  // Gets the window bounds according to the separator position.
  gfx::Rect GetLeftWindowBoundsInParent(aura::Window* window);
  gfx::Rect GetRightWindowBoundsInParent(aura::Window* window);
  gfx::Rect GetLeftWindowBoundsInScreen(aura::Window* window);
  gfx::Rect GetRightWindowBoundsInScreen(aura::Window* window);

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  // aura::WindowObserver overrides:
  void OnWindowDestroying(aura::Window* window) override;

  // ash::wm::WindowStateObserver overrides:
  void OnPostWindowStateTypeChange(ash::wm::WindowState* window_state,
                                   ash::wm::WindowStateType old_type) override;

  // wm::ActivationChangeObserver:
  void OnWindowActivated(ActivationReason reason,
                         aura::Window* gained_active,
                         aura::Window* lost_active) override;

  // ShellObserver overrides:
  void OnOverviewModeStarting() override;
  void OnOverviewModeEnded() override;

  aura::Window* left_window() { return left_window_; }
  aura::Window* right_window() { return right_window_; }
  int separator_position() { return separator_position_; }
  State state() { return state_; }

 private:
  enum DefaultSnapPosition { LEFT, RIGHT };

  // Ends the split view mode.
  void EndSplitView();

  // Stops observing |window| and restore its bounds.
  void StopObservingAndRestore(aura::Window* window);

  // Notify observers that the split view state has been changed.
  void NotifySplitViewStateChanged(State previous_state, State state);

  // The current left/right snapped window.
  aura::Window* left_window_ = nullptr;
  aura::Window* right_window_ = nullptr;

  // The x position of the separator between |left_window_| and |right_window_|.
  int separator_position_ = -1;

  // Current snap state.
  State state_ = NOSNAP;

  // The default snap position. It's decided by the first snapped window. If the
  // first window was snapped left, then |default_snap_position_| equals LEFT,
  // i.e., all the other windows will open snapped on the right side - And vice
  // versa.
  DefaultSnapPosition default_snap_position_ = LEFT;

  base::ObserverList<Observer> observers_;

  DISALLOW_COPY_AND_ASSIGN(SplitViewController);
};

}  // namespace ash

#endif  // ASH_WM_SPLITSVIEW_SPLIT_VIEW_CONTROLLER_H_
