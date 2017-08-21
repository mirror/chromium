// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_ACCESSIBILITY_ACCESSIBILITY_CONTROLLER_H_
#define ASH_ACCESSIBILITY_ACCESSIBILITY_CONTROLLER_H_

#include "ash/ash_export.h"
#include "base/macros.h"
#include "ash/session/session_observer.h"

class PrefRegistrySimple;

namespace ash {

// The controller for accessibility features in ash. Features can be enabled
// in chrome's webui settings or the system tray menu (see TrayAccessibility).
// Uses preference to communicate with chrome to support mash.
class ASH_EXPORT AccessibilityController : public SessionObserver {
 public:
  AccessibilityController();
  ~AccessibilityController() override;

  static void RegisterProfilePrefs(PrefRegistrySimple* registry);

  static void RegisterProfilePrefsForTesting(PrefRegistrySimple* registry);

  // Invoked to enable Large Cursor.
  void SetLargeCursorEnabled(bool enabled);

  // Returns true if Large Cursor is enabled.
  bool IsLargeCursorEnabled() const;

  // SessionObserver:
  void OnActiveUserPrefServiceChanged(PrefService* prefs) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(AccessibilityController);
};

}  // namespace ash

#endif  // ASH_ACCESSIBILITY_ACCESSIBILITY_CONTROLLER_H_
