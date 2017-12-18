// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/lifetime/application_lifetime_chromeos.h"

#include "base/barrier_closure.h"
#include "base/bind.h"
#include "base/callback.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/browser_shutdown.h"
#include "chrome/browser/chromeos/boot_times_recorder.h"
#include "chrome/browser/chromeos/profiles/profile_helper.h"
#include "chrome/browser/chromeos/settings/cros_settings.h"
#include "chrome/browser/download/download_core_service.h"
#include "chrome/browser/lifetime/application_lifetime.h"
#include "chrome/browser/lifetime/termination_notification.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/common/pref_names.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/power_manager_client.h"
#include "components/prefs/pref_service.h"
#include "components/user_manager/user_manager.h"
#include "content/public/browser/browser_thread.h"
#include "third_party/cros_system_api/dbus/service_constants.h"

namespace {

// Whether chrome should send stop request to a session manager.
bool g_send_stop_request_to_session_manager = false;

// If Chrome OS exit flow started flushing user data.
bool g_attempt_user_exit_chromeos_started = false;

// Sets kApplicationLocale in |local_state| for the login screen on the next
// application start, if it is forced to a specific value due to enterprise
// policy or the owner's locale.  Returns true if any pref has been modified.
bool SetLocaleForNextStart(PrefService* local_state) {
  // If a policy mandates the login screen locale, use it.
  chromeos::CrosSettings* cros_settings = chromeos::CrosSettings::Get();
  const base::ListValue* login_screen_locales = nullptr;
  std::string login_screen_locale;
  if (cros_settings->GetList(chromeos::kDeviceLoginScreenLocales,
                             &login_screen_locales) &&
      !login_screen_locales->empty() &&
      login_screen_locales->GetString(0, &login_screen_locale)) {
    local_state->SetString(prefs::kApplicationLocale, login_screen_locale);
    return true;
  }

  // Login screen should show up in owner's locale.
  std::string owner_locale = local_state->GetString(prefs::kOwnerLocale);
  if (!owner_locale.empty() &&
      local_state->GetString(prefs::kApplicationLocale) != owner_locale &&
      !local_state->IsManagedPreference(prefs::kApplicationLocale)) {
    local_state->SetString(prefs::kApplicationLocale, owner_locale);
    return true;
  }

  return false;
}

// Flush all logged-in users Prefs storage, and run |on_finish| callback when
// done.
void StartUserPrefsCommitPendingWrite(base::OnceClosure on_finish) {
  DCHECK(user_manager::UserManager::Get());
  const user_manager::UserList& users =
      user_manager::UserManager::Get()->GetLoggedInUsers();
  base::RepeatingClosure on_finish_barrier(
      base::BarrierClosure(users.size(), std::move(on_finish)));
  for (const user_manager::User* user : users) {
    Profile* profile = chromeos::ProfileHelper::Get()->GetProfileByUser(user);
    if (profile) {
      profile->GetPrefs()->CommitPendingWrite(on_finish_barrier);
    } else {
      // There is no prefs writer to trigger barrier for this profile.
      on_finish_barrier.Run();
    }
  }
}

void AttemptUserExitAfterFlush() {
  TRACE_EVENT0("shutdown", "NotifyAndTerminate");
  g_send_stop_request_to_session_manager = true;
  // On ChromeOS, always terminate the browser, regardless of the result of
  // AreAllBrowsersCloseable(). See https://crbug.com/123107.
  browser_shutdown::NotifyAndTerminate(true /* fast_path */);
}

}  // anonymous namespace

namespace chrome {
namespace platform {

// ChromeOS (ignore callback)
void AttemptUserExit(base::OnceClosure /* attempt_user_exit_finish */) {
  VLOG(1) << "AttemptUserExit";
  browser_shutdown::StartShutdownTracing();
  chromeos::BootTimesRecorder::Get()->AddLogoutTimeMarker("LogoutStarted",
                                                          false);

  PrefService* state = g_browser_process->local_state();
  if (state) {
    chromeos::BootTimesRecorder::Get()->OnLogoutStarted(state);

    if (SetLocaleForNextStart(state)) {
      TRACE_EVENT0("shutdown", "CommitPendingWrite");
      state->CommitPendingWrite();
    }
  }
  g_send_stop_request_to_session_manager = true;
  // On ChromeOS, always terminate the browser, regardless of the result of
  // AreAllBrowsersCloseable(). See https://crbug.com/123107.
  browser_shutdown::NotifyAndTerminate(true /* fast_path */);
}

void AttemptRestartFinish() {
  chromeos::BootTimesRecorder::Get()->set_restart_requested();

  DCHECK(!g_send_stop_request_to_session_manager);
  // Make sure we don't send stop request to the session manager.
  g_send_stop_request_to_session_manager = false;
  // Run exit process in clean stack.
  content::BrowserThread::PostTask(content::BrowserThread::UI, FROM_HERE,
                                   base::BindOnce(&chrome::ExitCleanly));
}

void PreAttemptExit() {
  // On ChromeOS, user exit and system exits are the same.
  chrome::AttemptUserExit();
}

void PreAttemptRelaunch() {
  chromeos::DBusThreadManager::Get()->GetPowerManagerClient()->RequestRestart(
      power_manager::REQUEST_RESTART_OTHER, "Chrome relaunch");
  // If running the Chrome OS build, but we're not on the device, fall through.
}

}  // namespace platform
}  // namespace chrome

namespace chromeos {

void AttemptUserExit() {
  DCHECK(!g_attempt_user_exit_chromeos_started);
  if (g_attempt_user_exit_chromeos_started)
    return;

  g_attempt_user_exit_chromeos_started = true;
  VLOG(1) << "AttemptUserExit";
  browser_shutdown::StartShutdownTracing();
  chromeos::BootTimesRecorder::Get()->AddLogoutTimeMarker("LogoutStarted",
                                                          false);

  PrefService* state = g_browser_process->local_state();
  if (state) {
    chromeos::BootTimesRecorder::Get()->OnLogoutStarted(state);

    if (SetLocaleForNextStart(state)) {
      TRACE_EVENT0("shutdown", "CommitPendingWrite");
      state->CommitPendingWrite();
    }
  }
  StartUserPrefsCommitPendingWrite(base::BindOnce(&AttemptUserExitAfterFlush));
}

bool IsAttemptingShutdown() {
  return g_send_stop_request_to_session_manager;
}

void PreExitCleanly() {
  VLOG(1) << "ExitCleanly";
  // We always mark exit cleanly.
  chrome::MarkAsCleanShutdown();

  // Don't block when SIGTERM is received. AreaAllBrowsersCloseable()
  // can be false in following cases. a) power-off b) signout from
  // screen locker.
  if (!chrome::AreAllBrowsersCloseable())
    browser_shutdown::OnShutdownStarting(browser_shutdown::END_SESSION);
  else
    browser_shutdown::OnShutdownStarting(browser_shutdown::BROWSER_EXIT);
}

}  // namespace chromeos
