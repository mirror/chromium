// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/ash/palette_delegate_chromeos.h"

#include "base/memory/ptr_util.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/chromeos/arc/voice_interaction/arc_voice_interaction_framework_service.h"
#include "chrome/browser/chromeos/note_taking_helper.h"
#include "chrome/browser/chromeos/profiles/profile_helper.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_service.h"
#include "components/user_manager/user_manager.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_source.h"

namespace chromeos {

PaletteDelegateChromeOS::PaletteDelegateChromeOS() : weak_factory_(this) {
  registrar_.Add(this, chrome::NOTIFICATION_SESSION_STARTED,
                 content::NotificationService::AllSources());
  registrar_.Add(this, chrome::NOTIFICATION_PROFILE_DESTROYED,
                 content::NotificationService::AllSources());
}

PaletteDelegateChromeOS::~PaletteDelegateChromeOS() {}

void PaletteDelegateChromeOS::CreateNote() {
  if (!profile_)
    return;

  chromeos::NoteTakingHelper::Get()->LaunchAppForNewNote(profile_,
                                                         base::FilePath());
}

bool PaletteDelegateChromeOS::HasNoteApp() {
  if (!profile_)
    return false;

  return chromeos::NoteTakingHelper::Get()->IsAppAvailable(profile_);
}

void PaletteDelegateChromeOS::ActiveUserChanged(
    const user_manager::User* active_user) {
  SetProfile(ProfileHelper::Get()->GetProfileByUser(active_user));
}

void PaletteDelegateChromeOS::Observe(
    int type,
    const content::NotificationSource& source,
    const content::NotificationDetails& details) {
  switch (type) {
    case chrome::NOTIFICATION_SESSION_STARTED:
      // Update |profile_| when entering a session.
      SetProfile(ProfileManager::GetActiveUserProfile());

      // Add a session state observer to be able to monitor session changes.
      if (!session_state_observer_.get()) {
        session_state_observer_.reset(
            new user_manager::ScopedUserSessionStateObserver(this));
      }
      break;
    case chrome::NOTIFICATION_PROFILE_DESTROYED: {
      // Update |profile_| when exiting a session or shutting down.
      Profile* profile = content::Source<Profile>(source).ptr();
      if (profile_ == profile)
        SetProfile(nullptr);
      break;
    }
  }
}

void PaletteDelegateChromeOS::SetProfile(Profile* profile) {
  profile_ = profile;
}

}  // namespace chromeos
