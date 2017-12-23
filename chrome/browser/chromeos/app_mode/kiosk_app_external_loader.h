// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_APP_MODE_KIOSK_APP_EXTERNAL_LOADER_H_
#define CHROME_BROWSER_CHROMEOS_APP_MODE_KIOSK_APP_EXTERNAL_LOADER_H_

#include "base/callback.h"
#include "base/macros.h"
#include "base/scoped_observer.h"
#include "chrome/browser/chromeos/app_mode/kiosk_app_manager_observer.h"
#include "chrome/browser/extensions/external_loader.h"

namespace chromeos {

class KioskAppManager;

// A custom extensions::ExternalLoader that is created by KioskAppManager and
// used for install kiosk app extensions.
class KioskAppExternalLoader : public extensions::ExternalLoader,
                               public KioskAppManagerObserver {
 public:
  enum class AppClass { PRIMARY, SECONDARY };

  explicit KioskAppExternalLoader(AppClass app_class);

  // extensions::ExternalLoader overrides:
  void StartLoading() override;

  // KioskAppManagerObserver:
  void OnPrimaryKioskAppReadyToLoad() override;
  void OnSecondaryKioskAppsReadyToLoad() override;

  using PrefsInterceptor = base::RepeatingCallback<
      void(bool initial_load, std::unique_ptr<base::DictionaryValue> prefs)>;
  void set_prefs_interceptor_for_testing(const PrefsInterceptor& interceptor) {
    prefs_interceptor_ = interceptor;
  }

 protected:
  ~KioskAppExternalLoader() override;

 private:
  enum class State { INITIAL, LOADING, LOADED };

  std::unique_ptr<base::DictionaryValue> GetAppsPrefs();
  void SendPrefsIfAvailable();

  const AppClass app_class_;
  State state_ = State::INITIAL;

  PrefsInterceptor prefs_interceptor_;

  ScopedObserver<KioskAppManager, KioskAppManagerObserver>
      kiosk_app_manager_observer_;

  DISALLOW_COPY_AND_ASSIGN(KioskAppExternalLoader);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_APP_MODE_KIOSK_APP_EXTERNAL_LOADER_H_
