// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_EXTENSIONS_EXTENSION_SYSTEM_EVENT_OBSERVER_H_
#define CHROME_BROWSER_CHROMEOS_EXTENSIONS_EXTENSION_SYSTEM_EVENT_OBSERVER_H_

#include "ash/session/session_observer.h"
#include "base/compiler_specific.h"
#include "base/macros.h"
#include "chromeos/dbus/power_manager_client.h"

namespace chromeos {

// Dispatches extension events in response to system events.
class ExtensionSystemEventObserver : public ash::SessionObserver,
                                     public PowerManagerClient::Observer {
 public:
  // This class registers/unregisters itself as an observer in ctor/dtor.
  ExtensionSystemEventObserver();
  ~ExtensionSystemEventObserver() override;

  // ash::SessionObserver override:
  void OnLockStateChanged(bool locked) override;

  // PowerManagerClient::Observer overrides:
  void BrightnessChanged(int level, bool user_initiated) override;
  void SuspendDone(const base::TimeDelta& sleep_duration) override;

 private:
  ash::ScopedSessionObserver session_observer_;

  DISALLOW_COPY_AND_ASSIGN(ExtensionSystemEventObserver);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_EXTENSIONS_EXTENSION_SYSTEM_EVENT_OBSERVER_H_
