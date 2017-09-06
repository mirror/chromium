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
#include "ui/display/display_observer.h"
#include "ui/gfx/geometry/point.h"
#include "ui/wm/public/activation_change_observer.h"

namespace ui {
class Layer;
}  // namespace ui

namespace ash {

class SplitViewControllerTest;
class SplitViewDivider;
class WindowSelectorTest;

// The controller for the split view. It snaps a window to left/right side of
// the screen. It also observes the two snapped windows and decides when to exit
// the split view mode.
class ASH_EXPORT SplitViewController : public aura::WindowObserver,
                                       public ash::wm::WindowStateObserver,
                                       public ::wm::ActivationChangeObserver,
                                       public ShellObserver,
                                       public display::DisplayObserver {
 public:
  enum State {
    NO_SNAP,
    LEFT_SNAPPED,
    RIGHT_SNAPPED,
    TOP_SNAPPED,
    BOTTOM_SNAPPED,
    BOTH_SNAPPED
  };
  enum SnapPosition { NONE, LEFT, RIGHT, TOP, BOTTOM };

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
  // mode is only supported in tablet mode (tablet mode).
  static bool ShouldAllowSplitView();

  // Returns true if split view mode is active.
  bool IsSplitViewModeActive() const;

  // Snaps window to left/right/top/bottom.
  void SnapWindow(aura::Window* window, SnapPosition snap_position);

  // Returns the default snapped window. It's the window that remains open until
  // the split mode ends. It's decided by |default_snap_position_|. E.g., If
  // |default_snap_position_| equals LEFT, then the default snapped window is
  // |left_window_|. All the other window will open on the right side.
  aura::Window* GetDefaultSnappedWindow();

  // Gets the opposite snap position for a giving |snap_position|.
  SnapPosition GetOppositeSnapPosition(SnapPosition snap_position);
  // Gets available snap position for a giving |snap_state|.
  SnapPosition GetAvailableSnapPositionFromSnapState(State snap_state);

  // Gets the window bounds according to the snap state |snap_state| and the
  // separator position |separator_position_|.
  gfx::Rect GetSnappedWindowBoundsInParent(aura::Window* window,
                                           SnapPosition snap_position);
  gfx::Rect GetSnappedWindowBoundsInScreen(aura::Window* window,
                                           SnapPosition snap_position);
  gfx::Rect GetDisplayWorkAreaBoundsInParent(aura::Window* window) const;
  gfx::Rect GetDisplayWorkAreaBoundsInScreen(aura::Window* window) const;

  void StartResize(const gfx::Point& location_in_screen);
  void Resize(const gfx::Point& location_in_screen);
  void EndResize(const gfx::Point& location_in_screen);

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  // aura::WindowObserver:
  void OnWindowDestroying(aura::Window* window) override;

  // ash::wm::WindowStateObserver:
  void OnPostWindowStateTypeChange(ash::wm::WindowState* window_state,
                                   ash::wm::WindowStateType old_type) override;

  // wm::ActivationChangeObserver:
  void OnWindowActivated(ActivationReason reason,
                         aura::Window* gained_active,
                         aura::Window* lost_active) override;

  // ShellObserver:
  void OnOverviewModeStarting() override;
  void OnOverviewModeEnded() override;

  // display::DisplayObserver:
  void OnDisplayMetricsChanged(const display::Display& display,
                               uint32_t metrics) override;

  aura::Window* left_window() { return left_window_; }
  aura::Window* right_window() { return right_window_; }
  int divider_position() const { return divider_position_; }
  State state() const { return state_; }
  SnapPosition default_snap_position() const { return default_snap_position_; }
  SplitViewDivider* split_view_divider() { return split_view_divider_.get(); }
  bool is_resizing() const { return is_resizing_; }

 private:
  friend class SplitViewControllerTest;
  friend class WindowSelectorTest;

  // Ends the split view mode.
  void EndSplitView();

  // Starts/Stops observing |window|.
  void StartObserving(aura::Window* window);
  void StopObserving(aura::Window* window);

  // Notifies observers that the split view state has been changed.
  void NotifySplitViewStateChanged(State previous_state, State state);

  // Gets the default value of |divider_position_| for a giving |snap_position|.
  int GetDefaultDividerPosition(const gfx::Rect& work_area_bounds_in_screen,
                                SnapPosition snap_position) const;

  // Updates the black scrim layer's bounds and opacity while dragging the
  // divider. The opacity increases as the split divider gets closer to the edge
  // of the screen.
  void UpdateBlackScrim(const gfx::Point& location_in_screen);

  // Restacks the two snapped windows while dragging the divider. If the divider
  // was in the left side of the screen, stack |right_window_| above
  // |left_window_|, otherwise, stack |left_window_| above |right_window_|. It's
  // necessary since we want the top window increasingly cover the entire
  // screen as the divider gets closer to the edge of the screen.
  void RestackWindows(const int previous_divider_position,
                      const int current_divider_position);

  // Update the bounds for the snapped windows and divider according to the
  // current snap direction.
  void UpdateSnappedWindowsAndDividerBounds();

  // The current left/top snapped window.
  aura::Window* left_window_ = nullptr;
  // The current right/bottom snapped window.
  aura::Window* right_window_ = nullptr;

  // Split view divider widget. It's a black bar stretching from one edge of the
  // screen to the other, containing a small white drag bar in the middle. As
  // the user presses on it and drag it to left or right, the left and right
  // window will be resized accordingly.
  std::unique_ptr<SplitViewDivider> split_view_divider_;

  // A black scrim layer that fades in over a window when it’s width drops under
  // 1/3 of the width of the screen, increasing in opacity as the divider gets
  // closer to the edge of the screen.
  std::unique_ptr<ui::Layer> black_scrim_layer_;

  // The x position (if horizontal) or y position (if vertical) of the divider
  // in screen coordinates.
  // TODO(xdai): Modify it to be the distance between the the divider and the
  // left window. Otherwise it will be hard to calculate the correct value when
  // display configuration is changed.
  int divider_position_ = -1;

  // The location of the previous mouse/gesture event in screen coordinates.
  gfx::Point previous_event_location_;

  // Current snap state.
  State state_ = NO_SNAP;

  // The default snap position. It's decided by the first snapped window. If the
  // first window was snapped left, then |default_snap_position_| equals LEFT,
  // i.e., all the other windows will open snapped on the right side - and vice
  // versa.
  SnapPosition default_snap_position_ = NONE;

  // If the divider is currently being dragging.
  bool is_resizing_ = false;

  base::ObserverList<Observer> observers_;

  DISALLOW_COPY_AND_ASSIGN(SplitViewController);
};

}  // namespace ash

#endif  // ASH_WM_SPLITSVIEW_SPLIT_VIEW_CONTROLLER_H_
