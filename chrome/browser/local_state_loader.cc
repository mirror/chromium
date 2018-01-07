// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/local_state_loader.h"

#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/sequenced_task_runner.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task_scheduler/post_task.h"
#include "chrome/browser/about_flags.h"
#include "chrome/browser/first_run/first_run.h"
#include "chrome/browser/policy/chrome_browser_policy_connector.h"
#include "chrome/browser/prefs/chrome_pref_service_factory.h"
#include "chrome/browser/prefs/browser_prefs.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/pref_names.h"
#include "chrome/installer/util/google_update_settings.h"
#include "components/flags_ui/pref_service_flags_storage.h"
#include "components/metrics/metrics_reporting_default_state.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "services/preferences/public/cpp/in_process_service_factory.h"

#if defined(OS_CHROMEOS)
#include "chrome/browser/chromeos/policy/browser_policy_connector_chromeos.h"
#include "chromeos/chromeos_switches.h"
#endif

namespace local_state_loader {
namespace {

std::unique_ptr<policy::BrowserPolicyConnector>
CreateBrowserPolicyConnector() {
#if defined(OS_CHROMEOS)
  return std::make_unique<policy::BrowserPolicyConnectorChromeOS>();
#else
  return std::make_unique<policy::ChromeBrowserPolicyConnector>();
#endif
}

void InitializeLocalState(LocalState* state) {
  // XXX rename this.
  TRACE_EVENT0("startup", "ChromeBrowserMainParts::InitializeLocalState")

#if defined(OS_WIN)
  if (first_run::IsChromeFirstRun()) {
    // During first run we read the google_update registry key to find what
    // language the user selected when downloading the installer. This
    // becomes our default language in the prefs.
    // Other platforms obey the system locale.
    base::string16 install_lang;
    if (GoogleUpdateSettings::GetLanguage(&install_lang)) {
      state->local_state->SetString(prefs::kApplicationLocale,
                                    base::UTF16ToASCII(install_lang));
    }
    bool stats_default;
    if (GoogleUpdateSettings::GetCollectStatsConsentDefault(&stats_default)) {
      // |stats_default| == true means that the default state of consent for the
      // product at the time of install was to report usage statistics, meaning
      // "opt-out".
      metrics::RecordMetricsReportingDefaultState(
          local_state, stats_default ? metrics::EnableMetricsDefault::OPT_OUT
                                     : metrics::EnableMetricsDefault::OPT_IN);
    }
  }
#endif  // defined(OS_WIN)

  // If the local state file for the current profile doesn't exist and the
  // parent profile command line flag is present, then we should inherit some
  // local state from the parent profile.
  // Checking that the local state file for the current profile doesn't exist
  // is the most robust way to determine whether we need to inherit or not
  // since the parent profile command line flag can be present even when the
  // current profile is not a new one, and in that case we do not want to
  // inherit and reset the user's setting.
  //
  // TODO(mnissler): We should probably just instantiate a
  // JSONPrefStore here instead of an entire PrefService. Once this is
  // addressed, the call to browser_prefs::RegisterLocalState can move
  // to chrome_prefs::CreateLocalState.
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  if (command_line->HasSwitch(switches::kParentProfile)) {
    base::FilePath local_state_path;
    PathService::Get(chrome::FILE_LOCAL_STATE, &local_state_path);
    bool local_state_file_exists = base::PathExists(local_state_path);
    if (!local_state_file_exists) {
      base::FilePath parent_profile =
          command_line->GetSwitchValuePath(switches::kParentProfile);
      scoped_refptr<PrefRegistrySimple> registry = new PrefRegistrySimple();
      std::unique_ptr<PrefService> parent_local_state(
          chrome_prefs::CreateLocalState(
              parent_profile, state->local_state_task_runner.get(),
              state->browser_policy_connector->GetPolicyService(),
              registry, false,
              state->browser_policy_connector.get(), nullptr));
      registry->RegisterStringPref(prefs::kApplicationLocale, std::string());
      // Right now, we only inherit the locale setting from the parent profile.
      state->local_state->SetString(
          prefs::kApplicationLocale,
          parent_local_state->GetString(prefs::kApplicationLocale));
    }
  }

#if defined(OS_CHROMEOS)
  if (command_line->HasSwitch(chromeos::switches::kLoginManager)) {
    std::string owner_locale = state->local_state->GetString(prefs::kOwnerLocale);
    // Ensure that we start with owner's locale.
    if (!owner_locale.empty() &&
        state->local_state->GetString(prefs::kApplicationLocale) != owner_locale &&
        !state->local_state->IsManagedPreference(prefs::kApplicationLocale)) {
      state->local_state->SetString(prefs::kApplicationLocale, owner_locale);
    }
  }
#endif  // defined(OS_CHROMEOS)
}

void ConvertFlagsToSwitches(PrefService* local_state) {
#if !defined(OS_CHROMEOS)
  // Convert active labs into switches. This needs to be done before
  // ui::ResourceBundle::InitSharedInstanceWithLocale as some loaded resources
  // are affected by experiment flags (--touch-optimized-ui in particular). On
  // ChromeOS system level flags are applied from the device settings from the
  // session manager.
  TRACE_EVENT0("startup",
               "ChromeBrowserMainParts::PreCreateThreadsImpl:ConvertFlags");
  flags_ui::PrefServiceFlagsStorage flags_storage(local_state);
  about_flags::ConvertFlagsToSwitches(&flags_storage,
                                      base::CommandLine::ForCurrentProcess(),
                                      flags_ui::kAddSentinels);
#endif  // !defined(OS_CHROMEOS)
}

}  // namespace

LocalState::LocalState() = default;

LocalState::~LocalState() = default;

std::unique_ptr<LocalState> CreateAndLoadLocalState() {
  base::FilePath user_data_dir;
  if (!PathService::Get(chrome::DIR_USER_DATA, &user_data_dir))
    return nullptr;

  std::unique_ptr<LocalState> result = std::make_unique<LocalState>();
  // The initial read is done synchronously, the TaskPriority is thus only used
  // for flushes to disks and BACKGROUND is therefore appropriate. Priority of
  // remaining BACKGROUND+BLOCK_SHUTDOWN tasks is bumped by the TaskScheduler on
  // shutdown. However, some shutdown use cases happen without
  // TaskScheduler::Shutdown() (e.g. ChromeRestartRequest::Start() and
  // BrowserProcessImpl::EndSession()) and we must thus unfortunately make this
  // USER_VISIBLE until we solve https://crbug.com/747495 to allow bumping
  // priority of a sequence on demand.
  result->local_state_task_runner =
      base::CreateSequencedTaskRunnerWithTraits(
          {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
           base::TaskShutdownBehavior::BLOCK_SHUTDOWN});

  result->pref_service_factory =
      base::MakeUnique<prefs::InProcessPrefServiceFactory>();
  result->browser_policy_connector = CreateBrowserPolicyConnector();

  base::FilePath local_state_path;
  CHECK(PathService::Get(chrome::FILE_LOCAL_STATE, &local_state_path));
  auto pref_registry = base::MakeRefCounted<PrefRegistrySimple>();

  // Register local state preferences.
  RegisterLocalState(pref_registry.get());

  auto delegate = result->pref_service_factory->CreateDelegate();
  delegate->InitPrefRegistry(pref_registry.get());
  result->local_state = chrome_prefs::CreateLocalState(
      local_state_path, result->local_state_task_runner.get(),
      result->browser_policy_connector->GetPolicyService(),
      pref_registry, false, result->browser_policy_connector.get(),
      std::move(delegate));
  DCHECK(result->local_state);

  InitializeLocalState(result.get());

  ConvertFlagsToSwitches(result->local_state.get());

  return result;
}

}  // namespace local_state_loader
