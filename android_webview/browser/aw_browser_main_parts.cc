// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "android_webview/browser/aw_browser_main_parts.h"

#include "android_webview/browser/aw_browser_context.h"
#include "android_webview/browser/aw_browser_terminator.h"
#include "android_webview/browser/aw_content_browser_client.h"
#include "android_webview/browser/aw_metrics_service_client.h"
#include "android_webview/browser/aw_result_codes.h"
#include "android_webview/browser/aw_safe_browsing_config_helper.h"
#include "android_webview/browser/deferred_gpu_command_service.h"
#include "android_webview/browser/net/aw_network_change_notifier_factory.h"
#include "android_webview/common/aw_descriptors.h"
#include "android_webview/common/aw_paths.h"
#include "android_webview/common/aw_resource.h"
#include "android_webview/common/aw_switches.h"
#include "android_webview/common/crash_reporter/aw_microdump_crash_reporter.h"
#include "base/android/apk_assets.h"
#include "base/android/build_info.h"
#include "base/android/locale_utils.h"
#include "base/android/memory_pressure_listener_android.h"
#include "base/base_switches.h"
#include "base/command_line.h"
#include "base/feature_list.h"
#include "base/files/file_path.h"
#include "base/guid.h"
#include "base/i18n/rtl.h"
#include "base/message_loop/message_loop.h"
#include "base/metrics/field_trial.h"
#include "base/path_service.h"
#include "cc/base/switches.h"
#include "components/crash/content/browser/crash_dump_manager_android.h"
#include "components/crash/content/browser/crash_dump_observer_android.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/tracing/common/tracing_switches.h"
#include "components/variations/caching_permuted_entropy_provider.h"
#include "components/variations/client_filterable_state.h"
#include "components/variations/field_trial_config/field_trial_util.h"
#include "components/variations/proto/variations_seed.pb.h"
#include "components/variations/service/variations_service.h"
#include "components/variations/variations_associated_data.h"
#include "components/variations/variations_http_header_provider.h"
#include "components/variations/variations_seed_store.h"
#include "components/variations/variations_switches.h"
#include "content/public/browser/android/synchronous_compositor.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/common/content_client.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/main_function_params.h"
#include "content/public/common/result_codes.h"
#include "device/geolocation/access_token_store.h"
#include "device/geolocation/geolocation_delegate.h"
#include "device/geolocation/geolocation_provider.h"
#include "net/android/network_change_notifier_factory_android.h"
#include "net/base/network_change_notifier.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/layout.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/resource/resource_bundle_android.h"
#include "ui/base/ui_base_paths.h"
#include "ui/gl/gl_surface.h"

namespace android_webview {
namespace {

class AwAccessTokenStore : public device::AccessTokenStore {
 public:
  AwAccessTokenStore() { }

  // device::AccessTokenStore implementation
  void LoadAccessTokens(const LoadAccessTokensCallback& request) override {
    AccessTokenStore::AccessTokenMap access_token_map;
    // AccessTokenMap and net::URLRequestContextGetter not used on Android,
    // but Run needs to be called to finish the geolocation setup.
    request.Run(access_token_map, NULL);
  }
  void SaveAccessToken(const GURL& server_url,
                       const base::string16& access_token) override {}

 private:
  ~AwAccessTokenStore() override {}

  DISALLOW_COPY_AND_ASSIGN(AwAccessTokenStore);
};

// A provider of Geolocation services to override AccessTokenStore.
class AwGeolocationDelegate : public device::GeolocationDelegate {
 public:
  AwGeolocationDelegate() = default;

  scoped_refptr<device::AccessTokenStore> CreateAccessTokenStore() final {
    return new AwAccessTokenStore();
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(AwGeolocationDelegate);
};

}  // anonymous namespace

AwBrowserMainParts::AwBrowserMainParts(AwContentBrowserClient* browser_client)
    : browser_client_(browser_client) {
}

AwBrowserMainParts::~AwBrowserMainParts() {
}

void AwBrowserMainParts::PreEarlyInitialization() {
  net::NetworkChangeNotifier::SetFactory(new AwNetworkChangeNotifierFactory());

  // Android WebView does not use default MessageLoop. It has its own
  // Android specific MessageLoop. Also see MainMessageLoopRun.
  DCHECK(!main_message_loop_.get());
  main_message_loop_.reset(new base::MessageLoopForUI);
  base::MessageLoopForUI::current()->Start();
}

std::unique_ptr<const base::FieldTrial::EntropyProvider> GetEntropyProvider() {
  base::FilePath user_data_dir;
  if (!PathService::Get(base::DIR_ANDROID_APP_DATA, &user_data_dir)) {
    NOTREACHED() << "Failed to get app data directory for Android WebView";
  }

  const base::FilePath guid_file_path =
      user_data_dir.Append(FILE_PATH_LITERAL("metrics_guid"));

  std::string guid = AwMetricsServiceClient::GetOrCreateGUID(guid_file_path);

  return std::unique_ptr<const base::FieldTrial::EntropyProvider>(
      new metrics::SHA1EntropyProvider(guid));
}

void AwBrowserMainParts::SetupFieldTrials() {
  field_trial_list_.reset(new base::FieldTrialList(GetEntropyProvider()));

  const base::CommandLine* command_line =
      base::CommandLine::ForCurrentProcess();
  if (command_line->HasSwitch(variations::switches::kEnableBenchmarking) ||
      command_line->HasSwitch(cc::switches::kEnableGpuBenchmarking)) {
    base::FieldTrial::EnableBenchmarking();
  }

  if (command_line->HasSwitch(variations::switches::kForceFieldTrialParams)) {
    bool result =
        variations::AssociateParamsFromString(command_line->GetSwitchValueASCII(
            variations::switches::kForceFieldTrialParams));
    CHECK(result) << "Invalid --"
                  << variations::switches::kForceFieldTrialParams
                  << " list specified.";
  }

  // Ensure any field trials specified on the command line are initialized.
  if (command_line->HasSwitch(switches::kForceFieldTrials)) {
    std::set<std::string> unforceable_field_trials;
#if defined(OFFICIAL_BUILD)
    unforceable_field_trials.insert("SettingsEnforcement");
#endif  // defined(OFFICIAL_BUILD)

    // Create field trials without activating them, so that this behaves in
    // a consistent manner with field trials created from the server.
    bool result = base::FieldTrialList::CreateTrialsFromString(
        command_line->GetSwitchValueASCII(switches::kForceFieldTrials),
        unforceable_field_trials);
    CHECK(result) << "Invalid --" << switches::kForceFieldTrials
                  << " list specified.";
  }

  std::unique_ptr<base::FeatureList> feature_list(new base::FeatureList);
  variations::VariationsHttpHeaderProvider* http_header_provider =
      variations::VariationsHttpHeaderProvider::GetInstance();
  // Force the variation ids selected in chrome://flags and/or specified using
  // the command-line flag.
  std::vector<std::string> variation_ids;
  bool result = http_header_provider->ForceVariationIds(
      command_line->GetSwitchValueASCII(
          variations::switches::kForceVariationIds),
      &variation_ids);
  CHECK(result) << "Invalid list of variation ids specified (either in --"
                << variations::switches::kForceVariationIds
                << " or in chrome://flags)";

  feature_list->InitializeFromCommandLine(
      command_line->GetSwitchValueASCII(switches::kEnableFeatures),
      command_line->GetSwitchValueASCII(switches::kDisableFeatures));

#if defined(FIELDTRIAL_TESTING_ENABLED)
  if (!command_line->HasSwitch(
          variations::switches::kDisableFieldTrialTestingConfig) &&
      !command_line->HasSwitch(switches::kForceFieldTrials) &&
      !command_line->HasSwitch(variations::switches::kVariationsServerURL)) {
    variations::AssociateDefaultFieldTrialConfig(feature_list.get());
  }
#endif  // defined(FIELDTRIAL_TESTING_ENABLED)

  // TODO: CreateTrialsFromSeed, Clank uses local_state
  // CreateTrialsFromSeedCommon();

  base::FeatureList::SetInstance(std::move(feature_list));
}

int AwBrowserMainParts::PreCreateThreads() {
  ui::SetLocalePaksStoredInApk(true);
  std::string locale = ui::ResourceBundle::InitSharedInstanceWithLocale(
      base::android::GetDefaultLocaleString(), NULL,
      ui::ResourceBundle::LOAD_COMMON_RESOURCES);
  if (locale.empty()) {
    LOG(WARNING) << "Failed to load locale .pak from the apk. "
        "Bringing up WebView without any locale";
  }
  base::i18n::SetICUDefaultLocale(locale);

  // Try to directly mmap the resources.pak from the apk. Fall back to load
  // from file, using PATH_SERVICE, otherwise.
  base::FilePath pak_file_path;
  PathService::Get(ui::DIR_RESOURCE_PAKS_ANDROID, &pak_file_path);
  pak_file_path = pak_file_path.AppendASCII("resources.pak");
  ui::LoadMainAndroidPackFile("assets/resources.pak", pak_file_path);

  base::android::MemoryPressureListenerAndroid::RegisterSystemCallback(
      base::android::AttachCurrentThread());
  DeferredGpuCommandService::SetInstance();
  breakpad::CrashDumpObserver::Create();

  if (crash_reporter::IsCrashReporterEnabled()) {
    base::FilePath crash_dir;
    if (PathService::Get(android_webview::DIR_CRASH_DUMPS, &crash_dir)) {
      if (!base::PathExists(crash_dir))
        base::CreateDirectory(crash_dir);
      breakpad::CrashDumpObserver::GetInstance()->RegisterClient(
          base::MakeUnique<breakpad::CrashDumpManager>(
              crash_dir, kAndroidMinidumpDescriptor));
    }
  }

  if (AwSafeBrowsingConfigHelper::GetSafeBrowsingEnabled()) {
    base::FilePath safe_browsing_dir;
    if (PathService::Get(android_webview::DIR_SAFE_BROWSING,
                         &safe_browsing_dir)) {
      if (!base::PathExists(safe_browsing_dir))
        base::CreateDirectory(safe_browsing_dir);
    }
  }

  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kWebViewSandboxedRenderer)) {
    // Create the renderers crash manager on the UI thread.
    breakpad::CrashDumpObserver::GetInstance()->RegisterClient(
        base::MakeUnique<AwBrowserTerminator>());
  }

  SetupFieldTrials();

  return content::RESULT_CODE_NORMAL_EXIT;
}

void AwBrowserMainParts::PreMainMessageLoopRun() {
  browser_client_->InitBrowserContext()->PreMainMessageLoopRun();

  device::GeolocationProvider::SetGeolocationDelegate(
      new AwGeolocationDelegate());

  content::RenderFrameHost::AllowInjectingJavaScriptForAndroidWebView();
}

bool AwBrowserMainParts::MainMessageLoopRun(int* result_code) {
  // Android WebView does not use default MessageLoop. It has its own
  // Android specific MessageLoop.
  return true;
}

}  // namespace android_webview
