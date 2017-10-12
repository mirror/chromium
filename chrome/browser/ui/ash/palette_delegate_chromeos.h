// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_ASH_PALETTE_DELEGATE_CHROMEOS_H_
#define CHROME_BROWSER_UI_ASH_PALETTE_DELEGATE_CHROMEOS_H_

#include <string>

#include "ash/palette_delegate.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/values.h"
#include "components/user_manager/user_manager.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"

class PrefChangeRegistrar;
class Profile;

namespace chromeos {

// A class which allows the Ash palette to perform chrome actions.
class PaletteDelegateChromeOS
    : public ash::PaletteDelegate,
      public user_manager::UserManager::UserSessionStateObserver,
      public content::NotificationObserver {
 public:
  PaletteDelegateChromeOS();
  ~PaletteDelegateChromeOS() override;

 private:
  // ash::PaletteDelegate:
  void CreateNote() override;
  bool HasNoteApp() override;
  void TakeScreenshot() override;
  void TakePartialScreenshot(const base::Closure& done) override;
  void CancelPartialScreenshot() override;

  // user_manager::UserManager::UserSessionStateObserver:
  void ActiveUserChanged(const user_manager::User* active_user) override;

  // content::NotificationObserver:
  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override;

  void SetProfile(Profile* profile);
  void OnPartialScreenshotDone(const base::Closure& then);

  // Unowned pointer to the active profile.
  Profile* profile_ = nullptr;
  std::unique_ptr<PrefChangeRegistrar> pref_change_registrar_;
  std::unique_ptr<user_manager::ScopedUserSessionStateObserver>
      session_state_observer_;
  content::NotificationRegistrar registrar_;

  base::WeakPtrFactory<PaletteDelegateChromeOS> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(PaletteDelegateChromeOS);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_UI_ASH_PALETTE_DELEGATE_CHROMEOS_H_
