// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/lock_screen_apps/profile_loader_impl.h"

#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/string16.h"
#include "base/time/tick_clock.h"
#include "base/time/time.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chromeos/profiles/profile_helper.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/safe_browsing/common/safe_browsing_prefs.h"
#include "extensions/browser/extension_system.h"
#include "extensions/common/one_shot_event.h"

namespace lock_screen_apps {

ProfileLoaderImpl::ProfileLoaderImpl(Profile* primary_profile,
                                     base::TickClock* tick_clock)
    : primary_profile_(primary_profile),
      tick_clock_(tick_clock),
      note_taking_helper_observer_(this),
      weak_ptr_factory_(this) {}

ProfileLoaderImpl::~ProfileLoaderImpl() {}

void ProfileLoaderImpl::OnAvailableNoteTakingAppsUpdated() {}

void ProfileLoaderImpl::OnPreferredNoteTakingAppUpdated(Profile* profile) {
  if (profile != primary_profile_)
    return;

  std::unique_ptr<chromeos::NoteTakingAppInfo> note_taking_app =
      chromeos::NoteTakingHelper::Get()->GetPreferredChromeAppInfo(
          primary_profile_);

  if (!note_taking_app || !note_taking_app->preferred ||
      note_taking_app->lock_screen_support !=
          chromeos::NoteTakingLockScreenSupport::kEnabled) {
    return;
  }

  note_taking_helper_observer_.RemoveAll();

  OnLockScreenProfileLoadStarted();

  g_browser_process->profile_manager()->CreateProfileAsync(
      chromeos::ProfileHelper::GetLockScreenAppProfilePath(),
      base::Bind(&ProfileLoaderImpl::OnProfileReady,
                 weak_ptr_factory_.GetWeakPtr(), tick_clock_->NowTicks()),
      base::string16() /* name */, "" /* icon_url*/,
      "" /* supervised_user_id */);
}

void ProfileLoaderImpl::InitializeImpl() {
  note_taking_helper_observer_.Add(chromeos::NoteTakingHelper::Get());

  // Preferred note taking app will be reported as null if it's note installed.
  // Make sure that extension system is ready (and extension regustry is loaded)
  // before testing for lock screen enabled app existence.
  extensions::ExtensionSystem::Get(primary_profile_)
      ->ready()
      .Post(FROM_HERE,
            base::Bind(&ProfileLoaderImpl::OnPreferredNoteTakingAppUpdated,
                       weak_ptr_factory_.GetWeakPtr(), primary_profile_));
}

void ProfileLoaderImpl::OnProfileReady(const base::TimeTicks& start_time,
                                       Profile* profile,
                                       Profile::CreateStatus status) {
  // Ignore CREATED status - wait for profile to be initialized before
  // continuing.
  if (status == Profile::CREATE_STATUS_CREATED) {
    // Disable safe browsing for the profile to avoid activating
    // SafeBrowsingService when the user has safe browsing disabled (reasoning
    // similar to http://crbug.com/461493).
    // TODO(tbarzic): Revisit this if webviews get enabled for lock screen apps.
    profile->GetPrefs()->SetBoolean(prefs::kSafeBrowsingEnabled, false);
    return;
  }

  UMA_HISTOGRAM_BOOLEAN("Apps.LockScreen.LockScreenAppsProfileCreationSuccess",
                        status == Profile::CREATE_STATUS_INITIALIZED);

  // On error, bail out - this will cause the lock screen apps to remain
  // unavailable on the device.
  if (status != Profile::CREATE_STATUS_INITIALIZED) {
    OnLockScreenProfileLoaded(nullptr);
    return;
  }

  profile->GetPrefs()->SetBoolean(prefs::kForceEphemeralProfiles, true);

  UMA_HISTOGRAM_TIMES("Apps.LockScreen.TimeToCreateLockScreenAppsProfile",
                      tick_clock_->NowTicks() - start_time);

  OnLockScreenProfileLoaded(profile);
}

}  // namespace lock_screen_apps
