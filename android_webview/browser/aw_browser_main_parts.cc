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
#include "android_webview/browser/aw_variations_service_client.h"
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
#include "base/i18n/rtl.h"
#include "base/message_loop/message_loop.h"
#include "base/path_service.h"
#include "cc/base/switches.h"
#include "components/crash/content/browser/crash_dump_manager_android.h"
#include "components/crash/content/browser/crash_dump_observer_android.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/in_memory_pref_store.h"
#include "components/prefs/pref_registry.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/pref_service_factory.h"
#include "components/variations/pref_names.h"
#include "content/public/browser/android/synchronous_compositor.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/common/content_client.h"
#include "content/public/common/content_switches.h"
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

std::unique_ptr<const base::FieldTrial::EntropyProvider>
CreateLowEntropyProvider() {
  return std::unique_ptr<const base::FieldTrial::EntropyProvider>(
      new metrics::SHA1EntropyProvider(
          android_webview::AwMetricsServiceClient::GetClientID()));
}

// Synchronous read of variations data is allowed for now. See discussion at:
// https://groups.google.com/a/google.com/d/topic/webview-finch-project/YuX7HlF1vMw/discussion
bool ReadVariationsSeedDataFromFile(PrefService* local_state) {
  base::FilePath user_data_dir;
  if (!PathService::Get(base::DIR_ANDROID_APP_DATA, &user_data_dir)) {
    LOG(ERROR) << "Failed to get app data directory for Android WebView";
    return false;
  }

  const base::FilePath variations_seed_path =
      user_data_dir.Append(FILE_PATH_LITERAL("variations_seed_data"));
  const base::FilePath variations_pref_path =
      user_data_dir.Append(FILE_PATH_LITERAL("variations_seed_pref"));

  // Set compressed seed data.
  std::string tmp_str;
  if (!base::ReadFileToString(variations_seed_path, &tmp_str)) {
    LOG(ERROR) << "Failed to read variations seed data";
    return false;
  }
  local_state->SetString(variations::prefs::kVariationsCompressedSeed, tmp_str);

  // Set seed meta-data.
  if (!base::ReadFileToString(variations_pref_path, &tmp_str)) {
    LOG(ERROR) << "Failed to read variations seed meta-data";
    return false;
  }
  std::vector<std::string> tokens = base::SplitString(
      tmp_str, "\n", base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL);

  local_state->SetString(variations::prefs::kVariationsSeedSignature,
                         tokens[0]);
  local_state->SetString(variations::prefs::kVariationsCountry, tokens[1]);

  base::Time fetch_date;
  if (!base::Time::FromUTCString(tokens[2].c_str(), &fetch_date)) {
    LOG(ERROR) << "Failed to parse seed last fetch date from string";
    return false;
  }
  local_state->SetInt64(variations::prefs::kVariationsSeedDate,
                        fetch_date.ToInternalValue());

  // Say the seed was last fetched now, so it will not fetch from the
  // server.
  local_state->SetInt64(variations::prefs::kVariationsLastFetchTime,
                        base::Time::Now().ToInternalValue());

  return true;
}

void AwBrowserMainParts::SetUpFieldTrials() {
  // UMA is only enabled on Android N+, don't enable Finch without UMA.
  if (base::android::BuildInfo::GetInstance()->sdk_int() <
      base::android::SDK_VERSION_NOUGAT) {
    return;
  }

  DCHECK(!field_trial_list_);
  field_trial_list_.reset(new base::FieldTrialList(CreateLowEntropyProvider()));

  //-----------------------------------------------------------------------------
  user_prefs::PrefRegistrySyncable* pref_registry =
      new user_prefs::PrefRegistrySyncable();
  pref_registry->RegisterInt64Pref(variations::prefs::kVariationsSeedDate, 0);
  pref_registry->RegisterInt64Pref(variations::prefs::kVariationsLastFetchTime,
                                   0);
  pref_registry->RegisterStringPref(variations::prefs::kVariationsCountry,
                                    "us");
  pref_registry->RegisterStringPref(
      variations::prefs::kVariationsCompressedSeed, "");
  pref_registry->RegisterStringPref(variations::prefs::kVariationsSeedSignature,
                                    "");
  std::unique_ptr<base::ListValue> default_value(new base::ListValue);
  pref_registry->RegisterListPref(
      variations::prefs::kVariationsPermanentConsistencyCountry,
      std::move(default_value));

  PrefServiceFactory pref_service_factory;
  pref_service_factory.set_user_prefs(
      make_scoped_refptr(new InMemoryPrefStore()));
  std::unique_ptr<PrefService> local_state =
      pref_service_factory.Create(pref_registry);

  if (!ReadVariationsSeedDataFromFile(local_state.get())) {
    return;
  }

  variations::UIStringOverrider ui_string_overrider;
  std::unique_ptr<AwVariationsServiceClient> client(
      new AwVariationsServiceClient);
  variations_field_trial_creator_.reset(
      new variations::VariationsFieldTrialCreator(
          local_state.get(), client.get(), ui_string_overrider));

  variations_field_trial_creator_->OverrideVariationsPlatform(
      variations::Study::PLATFORM_ANDROID_WEBVIEW);

  std::unique_ptr<base::FeatureList> feature_list(new base::FeatureList);
  std::vector<std::string> variation_ids;
  std::set<std::string> unforceable_field_trials;

  variations_field_trial_creator_->SetupFieldTrials(
      cc::switches::kEnableGpuBenchmarking, switches::kEnableFeatures,
      switches::kDisableFeatures, unforceable_field_trials,
      CreateLowEntropyProvider(), std::move(feature_list), &variation_ids,
      &webview_field_trials_);
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

  // We need to create the safe browsing specific directory even if the
  // AwSafeBrowsingConfigHelper::GetSafeBrowsingEnabled() is false
  // initially, because safe browsing can be enabled later at runtime
  // on a per-webview basis.
  base::FilePath safe_browsing_dir;
  if (PathService::Get(android_webview::DIR_SAFE_BROWSING,
                       &safe_browsing_dir)) {
    if (!base::PathExists(safe_browsing_dir))
      base::CreateDirectory(safe_browsing_dir);
  }

  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kWebViewSandboxedRenderer)) {
    // Create the renderers crash manager on the UI thread.
    breakpad::CrashDumpObserver::GetInstance()->RegisterClient(
        base::MakeUnique<AwBrowserTerminator>());
  }

  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kEnableWebViewFinch)) {
    SetUpFieldTrials();
  }

  return content::RESULT_CODE_NORMAL_EXIT;
}

void AwBrowserMainParts::PreMainMessageLoopRun() {
  browser_client_->InitBrowserContext()->PreMainMessageLoopRun();

  device::GeolocationProvider::SetGeolocationDelegate(
      new AwGeolocationDelegate());

  content::RenderFrameHost::AllowInjectingJavaScriptForAndroidWebView();

  // TODO(meacer): Remove when PlzNavigate ships.
  content::RenderFrameHost::AllowDataUrlNavigationForAndroidWebView();

  // This only works because webview uses in-process gpu
  // which is not started up early by BrowserMainLoop.
  DeferredGpuCommandService::SetInstance();
}

bool AwBrowserMainParts::MainMessageLoopRun(int* result_code) {
  // Android WebView does not use default MessageLoop. It has its own
  // Android specific MessageLoop.
  return true;
}

}  // namespace android_webview
