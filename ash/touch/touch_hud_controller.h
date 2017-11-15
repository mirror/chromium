// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_TOUCH_TOUCH_HUD_CONTROLLER_H_
#define ASH_TOUCH_TOUCH_HUD_CONTROLLER_H_

#include "ash/ash_export.h"
#include "ash/session/session_observer.h"
#include "base/macros.h"

class PrefRegistrySimple;

namespace ash {

// Controls whether the touch HUD is enabled or disabled. The touch HUD draws
// a circle for each touch tap. The HUD is used for demos/presentations because
// there is no visible cursor to show where taps occur. This class exists
// because other touch HUD classes are per-display, but the feature is enabled
// globally.
class ASH_EXPORT TouchHudController : public SessionObserver {
 public:
  TouchHudController();
  ~TouchHudController() override;

  static void RegisterProfilePrefs(PrefRegistrySimple* registry);

  bool IsTouchHudProjectionEnabled() const;
  void SetTouchHudProjectionEnabled(bool enabled);

  // SessionObserver:
  void OnActiveUserPrefServiceChanged(PrefService* prefs) override;

 private:
  ScopedSessionObserver session_observer_;

  DISALLOW_COPY_AND_ASSIGN(TouchHudController);
};

}  // namespace ash

#endif  // ASH_TOUCH_TOUCH_HUD_CONTROLLER_H_
