// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_LOCK_SCREEN_ACTION_LOCK_SCREEN_ACTION_BACKGROUND_CONTROLLER_H_
#define ASH_SYSTEM_LOCK_SCREEN_ACTION_LOCK_SCREEN_ACTION_BACKGROUND_CONTROLLER_H_

#include "ash/system/lock_screen_action/lock_screen_action_background_state.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"

namespace aura {
class Window;
}

namespace ash {

class LockScreenActionBackgroundObserver;

// Base class for managing lock screen action background widget. The
// implementation should provide methods for showing and hiding background
// widget when requested.
// The base class manages background widget state change observers, and keeps
// track of the current background widget state.
class LockScreenActionBackgroundController {
 public:
  LockScreenActionBackgroundController();
  virtual ~LockScreenActionBackgroundController();

  // Sets the window the background widget should use as its parent.
  void SetParentWindow(aura::Window* parent_window);

  void AddObserver(LockScreenActionBackgroundObserver* observer);
  void RemoveObserver(LockScreenActionBackgroundObserver* observer);

  LockScreenActionBackgroundState state() const { return state_; }

  // Returns whether |window| is the background widget window.
  virtual bool IsBackgroundWindow(aura::Window* window) const = 0;

  // Starts showing background widget.
  //
  // Returns whether the background widget state has changed. On success, the
  // state is expected to change to either kShowing, or kShown.
  virtual bool ShowBackground() = 0;

  // Starts hiding the background widget. Unlike |HideBackgroundImedatelly|,
  // hiding the widget may be accompanied with an animation, in which case state
  // should be changed to kHiding.
  //
  // Returns whether the background widget state has changed. On success, the
  // state is expected to change to either kHiding or kHidden.
  virtual bool HideBackground() = 0;

  // Hides background widget without any animation.
  //
  // Returns whether the background widget state has changed. On success the
  // state is expected to change to kHidden.
  virtual bool HideBackgroundImmediately() = 0;

 protected:
  void UpdateState(LockScreenActionBackgroundState state);

  aura::Window* parent_window_ = nullptr;

 private:
  LockScreenActionBackgroundState state_ =
      LockScreenActionBackgroundState::kHidden;

  base::ObserverList<LockScreenActionBackgroundObserver> observers_;

  DISALLOW_COPY_AND_ASSIGN(LockScreenActionBackgroundController);
};

}  // namespace ash

#endif  // ASH_SYSTEM_LOCK_SCREEN_ACTION_LOCK_SCREEN_ACTION_BACKGROUND_CONTROLLER_H_
