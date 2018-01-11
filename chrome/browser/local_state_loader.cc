// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/local_state_loader.h"

#include "base/base_switches.h"
#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/metrics/field_trial.h"
#include "base/path_service.h"
#include "base/sequenced_task_runner.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task_scheduler/post_task.h"
#include "cc/base/switches.h"
#include "chrome/browser/about_flags.h"
#include "chrome/browser/chrome_browser_field_trials.h"
#include "chrome/browser/first_run/first_run.h"
#include "chrome/browser/metrics/chrome_metrics_services_manager_client.h"
#include "chrome/browser/metrics/field_trial_synchronizer.h"
#include "chrome/browser/policy/chrome_browser_policy_connector.h"
#include "chrome/browser/prefs/browser_prefs.h"
#include "chrome/browser/prefs/chrome_pref_service_factory.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/chrome_result_codes.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/pref_names.h"
#include "chrome/installer/util/google_update_settings.h"
#include "components/flags_ui/pref_service_flags_storage.h"
#include "components/metrics/metrics_reporting_default_state.h"
#include "components/metrics_services_manager/metrics_services_manager.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/tracing/common/tracing_switches.h"
#include "components/variations/pref_names.h"
#include "components/variations/service/variations_service.h"
#include "services/preferences/public/cpp/in_process_service_factory.h"

#if defined(OS_CHROMEOS)
#include "chrome/browser/chromeos/policy/browser_policy_connector_chromeos.h"
#include "chrome/browser/chromeos/settings/cros_settings.h"
#include "chromeos/chromeos_switches.h"
#endif

namespace local_state_loader {
namespace {

std::unique_ptr<policy::ChromeBrowserPolicyConnector>
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
          state->local_state.get(),
          stats_default ? metrics::EnableMetricsDefault::OPT_OUT
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
              state->browser_policy_connector->GetPolicyService(), registry,
              false, state->browser_policy_connector.get(), nullptr));
      registry->RegisterStringPref(prefs::kApplicationLocale, std::string());
      // Right now, we only inherit the locale setting from the parent profile.
      state->local_state->SetString(
          prefs::kApplicationLocale,
          parent_local_state->GetString(prefs::kApplicationLocale));
    }
  }

#if defined(OS_CHROMEOS)
  if (command_line->HasSwitch(chromeos::switches::kLoginManager)) {
    std::string owner_locale =
        state->local_state->GetString(prefs::kOwnerLocale);
    // Ensure that we start with owner's locale.
    if (!owner_locale.empty() &&
        state->local_state->GetString(prefs::kApplicationLocale) !=
            owner_locale &&
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

int ApplyFirstRunPrefs(const base::FilePath& user_data_dir, LocalState* state) {
#if !defined(OS_ANDROID)
  state->master_prefs = std::make_unique<first_run::MasterPrefs>();
#endif

// Android does first run in Java instead of native.
// Chrome OS has its own out-of-box-experience code.
#if !defined(OS_ANDROID) && !defined(OS_CHROMEOS)
  // On first run, we need to process the predictor preferences before the
  // browser's profile_manager object is created, but after ResourceBundle
  // is initialized.
  if (!first_run::IsChromeFirstRun())
    return content::RESULT_CODE_NORMAL_EXIT;

  first_run::ProcessMasterPreferencesResult pmp_result =
      first_run::ProcessMasterPreferences(user_data_dir,
                                          state->master_prefs.get());
  if (pmp_result == first_run::EULA_EXIT_NOW)
    return chrome::RESULT_CODE_EULA_REFUSED;

  // TODO(macourteau): refactor preferences that are copied from
  // master_preferences into local_state, as a "local_state" section in
  // master preferences. If possible, a generic solution would be preferred
  // over a copy one-by-one of specific preferences. Also see related TODO
  // in first_run.h.

  // Store the initial VariationsService seed in local state, if it exists
  // in master prefs.
  if (!state->master_prefs->compressed_variations_seed.empty()) {
    state->local_state->SetString(
        variations::prefs::kVariationsCompressedSeed,
        state->master_prefs->compressed_variations_seed);
    if (!state->master_prefs->variations_seed_signature.empty()) {
      state->local_state->SetString(
          variations::prefs::kVariationsSeedSignature,
          state->master_prefs->variations_seed_signature);
    }
    // Set the variation seed date to the current system time. If the user's
    // clock is incorrect, this may cause some field trial expiry checks to
    // not do the right thing until the next seed update from the server,
    // when this value will be updated.
    state->local_state->SetInt64(variations::prefs::kVariationsSeedDate,
                                 base::Time::Now().ToInternalValue());
  }

  if (!state->master_prefs->suppress_default_browser_prompt_for_version
           .empty()) {
    state->local_state->SetString(
        prefs::kBrowserSuppressDefaultBrowserPrompt,
        state->master_prefs->suppress_default_browser_prompt_for_version);
  }

#if defined(OS_WIN)
  if (!state->master_prefs->welcome_page_on_os_upgrade_enabled)
    state->local_state->SetBoolean(prefs::kWelcomePageOnOSUpgradeEnabled,
                                   false);
#endif
#endif  // !defined(OS_ANDROID) && !defined(OS_CHROMEOS)
  return content::RESULT_CODE_NORMAL_EXIT;
}

void SetupOriginTrialsCommandLine(PrefService* local_state) {
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  if (!command_line->HasSwitch(switches::kOriginTrialPublicKey)) {
    std::string new_public_key =
        local_state->GetString(prefs::kOriginTrialPublicKey);
    if (!new_public_key.empty()) {
      command_line->AppendSwitchASCII(
          switches::kOriginTrialPublicKey,
          local_state->GetString(prefs::kOriginTrialPublicKey));
    }
  }
  if (!command_line->HasSwitch(switches::kOriginTrialDisabledFeatures)) {
    const base::ListValue* override_disabled_feature_list =
        local_state->GetList(prefs::kOriginTrialDisabledFeatures);
    if (override_disabled_feature_list) {
      std::vector<base::StringPiece> disabled_features;
      base::StringPiece disabled_feature;
      for (const auto& item : *override_disabled_feature_list) {
        if (item.GetAsString(&disabled_feature)) {
          disabled_features.push_back(disabled_feature);
        }
      }
      if (!disabled_features.empty()) {
        const std::string override_disabled_features =
            base::JoinString(disabled_features, "|");
        command_line->AppendSwitchASCII(switches::kOriginTrialDisabledFeatures,
                                        override_disabled_features);
      }
    }
  }
  if (!command_line->HasSwitch(switches::kOriginTrialDisabledTokens)) {
    const base::ListValue* disabled_token_list =
        local_state->GetList(prefs::kOriginTrialDisabledTokens);
    if (disabled_token_list) {
      std::vector<base::StringPiece> disabled_tokens;
      base::StringPiece disabled_token;
      for (const auto& item : *disabled_token_list) {
        if (item.GetAsString(&disabled_token)) {
          disabled_tokens.push_back(disabled_token);
        }
      }
      if (!disabled_tokens.empty()) {
        const std::string disabled_token_switch =
            base::JoinString(disabled_tokens, "|");
        command_line->AppendSwitchASCII(switches::kOriginTrialDisabledTokens,
                                        disabled_token_switch);
      }
    }
  }
}

void SetupFieldTrials(LocalState* state) {
  auto client = std::make_unique<ChromeMetricsServicesManagerClient>(
      state->local_state.get());
  state->metrics_services_manager =
      std::make_unique<metrics_services_manager::MetricsServicesManager>(
          std::move(client));

  // Initialize FieldTrialList to support FieldTrials that use one-time
  // randomization.
  state->field_trial_list = std::make_unique<base::FieldTrialList>(
      state->metrics_services_manager->CreateEntropyProvider());

  std::unique_ptr<base::FeatureList> feature_list =
      std::make_unique<base::FeatureList>();

  // Associate parameters chosen in about:flags and create trial/group for them.
  flags_ui::PrefServiceFlagsStorage flags_storage(state->local_state.get());
  std::vector<std::string> variation_ids =
      about_flags::RegisterAllFeatureVariationParameters(&flags_storage,
                                                         feature_list.get());

  std::set<std::string> unforceable_field_trials;
#if defined(OFFICIAL_BUILD)
  unforceable_field_trials.insert("SettingsEnforcement");
#endif  // defined(OFFICIAL_BUILD)

  variations::VariationsService* variations_service =
      state->metrics_services_manager->GetVariationsService();
  state->browser_field_trials = std::make_unique<ChromeBrowserFieldTrials>();
  variations_service->SetupFieldTrials(
      cc::switches::kEnableGpuBenchmarking, switches::kEnableFeatures,
      switches::kDisableFeatures, unforceable_field_trials,
      std::move(feature_list), &variation_ids,
      state->browser_field_trials.get());

  // See comment in here for lifetime.
  state->field_trial_synchronizer = new FieldTrialSynchronizer();
}

}  // namespace

LocalState::LocalState() = default;

LocalState::~LocalState() = default;

int LoadLocalState(LocalState* result) {
  base::FilePath user_data_dir;
  if (!PathService::Get(chrome::DIR_USER_DATA, &user_data_dir))
    return chrome::RESULT_CODE_MISSING_DATA;

  // The initial read is done synchronously, the TaskPriority is thus only used
  // for flushes to disks and BACKGROUND is therefore appropriate. Priority of
  // remaining BACKGROUND+BLOCK_SHUTDOWN tasks is bumped by the TaskScheduler on
  // shutdown. However, some shutdown use cases happen without
  // TaskScheduler::Shutdown() (e.g. ChromeRestartRequest::Start() and
  // BrowserProcessImpl::EndSession()) and we must thus unfortunately make this
  // USER_VISIBLE until we solve https://crbug.com/747495 to allow bumping
  // priority of a sequence on demand.
  result->local_state_task_runner = base::CreateSequencedTaskRunnerWithTraits(
      {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
       base::TaskShutdownBehavior::BLOCK_SHUTDOWN});

  internal::CreateLocalState(
      result->local_state_task_runner, &result->pref_service_factory,
      &result->browser_policy_connector, &result->local_state);

  InitializeLocalState(result);

  ConvertFlagsToSwitches(result->local_state.get());

  const int apply_first_run_result = ApplyFirstRunPrefs(user_data_dir, result);
  if (apply_first_run_result != content::RESULT_CODE_NORMAL_EXIT)
    return apply_first_run_result;

  SetupOriginTrialsCommandLine(result->local_state.get());

  // Now that the command line has been mutated based on about:flags, we can
  // initialize field trials. The field trials are needed by IOThread's
  // initialization which happens in BrowserProcess:PreCreateThreads. Metrics
  // initialization is handled in PreMainMessageLoopRunImpl since it posts
  // tasks.
  SetupFieldTrials(result);

  return content::RESULT_CODE_NORMAL_EXIT;
}

namespace internal {

void CreateLocalState(
    scoped_refptr<base::SequencedTaskRunner> local_state_task_runner,
    std::unique_ptr<prefs::InProcessPrefServiceFactory>* pref_service_factory,
    std::unique_ptr<policy::ChromeBrowserPolicyConnector>*
        browser_policy_connector,
    std::unique_ptr<PrefService>* local_state) {
  *pref_service_factory =
      base::MakeUnique<prefs::InProcessPrefServiceFactory>();
  *browser_policy_connector = CreateBrowserPolicyConnector();

  base::FilePath local_state_path;
  CHECK(PathService::Get(chrome::FILE_LOCAL_STATE, &local_state_path));
  auto pref_registry = base::MakeRefCounted<PrefRegistrySimple>();

  // Register local state preferences.
  RegisterLocalState(pref_registry.get());

  auto delegate = (*pref_service_factory)->CreateDelegate();
  delegate->InitPrefRegistry(pref_registry.get());
  *local_state = chrome_prefs::CreateLocalState(
      local_state_path, local_state_task_runner.get(),
      (*browser_policy_connector)->GetPolicyService(), pref_registry, false,
      browser_policy_connector->get(), std::move(delegate));
  DCHECK(*local_state);
}

}  // namespace internal

}  // namespace local_state_loader
