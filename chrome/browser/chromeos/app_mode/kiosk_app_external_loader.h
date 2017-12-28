// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_APP_MODE_KIOSK_APP_EXTERNAL_LOADER_H_
#define CHROME_BROWSER_CHROMEOS_APP_MODE_KIOSK_APP_EXTERNAL_LOADER_H_

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/extensions/external_loader.h"

namespace chromeos {

// A custom extensions::ExternalLoader that loads apps run in a kiosk session.
// It an be used to either load primary kiosk app, or secondary kiosk apps (or
// extensions), not for both.
// It registers a callback with KioskAppManager that is run when the relevant
// kiosk apps' information is updated, at which time they can be loaded.
// This class may update the set of extensions it provides - e.g. if an offline
// enabled kiosk app gets updated during its initial launch (if the launch
// started while the device was offline).
// This class lives on the UI thread, even though it's ref counted as a subclass
// of |extensions::ExternalLoader|.
class KioskAppExternalLoader : public extensions::ExternalLoader {
 public:
  enum class AppClass { PRIMARY, SECONDARY };

  explicit KioskAppExternalLoader(AppClass app_class);

  // extensions::ExternalLoader overrides:
  void StartLoading() override;

 protected:
  ~KioskAppExternalLoader() override;

 private:
  enum class State { INITIAL, LOADING, LOADED };

  // Gets prefs describing appropriate set of external extensions (depending
  // on the class of kiosk apps handled by |this|) from kiosk app manager.
  std::unique_ptr<base::DictionaryValue> GetAppsPrefs();

  // If prefs for the set of kiosk apps handled by |this| are available, sends
  // them to the external loader owner (using extensions::ExternalLoader
  // interface).
  void SendPrefsIfAvailable();

  // The class of kiosk apps this external loader handles.
  const AppClass app_class_;

  // Current kiosk app external loader state.
  State state_ = State::INITIAL;

  base::WeakPtrFactory<KioskAppExternalLoader> weak_ptr_factory_{this};

  DISALLOW_COPY_AND_ASSIGN(KioskAppExternalLoader);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_APP_MODE_KIOSK_APP_EXTERNAL_LOADER_H_
