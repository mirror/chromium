// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/app_mode/startup_app_launcher.h"

#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "base/callback.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/run_loop.h"
#include "base/test/scoped_command_line.h"
#include "base/version.h"
#include "chrome/browser/chromeos/app_mode/app_session.h"
#include "chrome/browser/chromeos/app_mode/kiosk_app_external_loader.h"
#include "chrome/browser/chromeos/app_mode/kiosk_app_launch_error.h"
#include "chrome/browser/chromeos/app_mode/kiosk_app_manager.h"
#include "chrome/browser/chromeos/extensions/test_external_cache.h"
#include "chrome/browser/chromeos/policy/device_local_account.h"
#include "chrome/browser/chromeos/settings/device_settings_test_helper.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/extensions/external_provider_impl.h"
#include "chrome/browser/extensions/install_tracker.h"
#include "chrome/browser/extensions/pending_extension_manager.h"
#include "chrome/browser/extensions/test_extension_system.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/test/base/scoped_testing_local_state.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "chromeos/settings/cros_settings_names.h"
#include "components/session_manager/core/session_manager.h"
#include "components/signin/core/account_id/account_id.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "extensions/browser/test_event_router.h"
#include "extensions/common/api/app_runtime.h"
#include "extensions/common/extension_builder.h"
#include "extensions/common/value_builder.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

using extensions::DictionaryBuilder;
using extensions::ExtensionBuilder;
using extensions::ListBuilder;

namespace {

constexpr char kTestPrimaryAppId[] = "abcdefghabcdefghabcdefghabcdefgh";

enum class LaunchState {
  kNotStarted,
  kInitializingNetwork,
  kInstallingApp,
  kReadyToLaunch,
  kLaunchSucceeded,
  kLaunchFailed
};

class AppLaunchTracker : public extensions::TestEventRouter::EventObserver {
 public:
  AppLaunchTracker(const std::string& app_id,
                   extensions::TestEventRouter* event_router)
      : app_id_(app_id), event_router_(event_router) {
    event_router->AddEventObserver(this);
  }
  ~AppLaunchTracker() override { event_router_->RemoveEventObserver(this); }

  int kiosk_launch_count() const { return kiosk_launch_count_; }

  // TestEventRouter::EventObserver:
  void OnBroadcastEvent(const extensions::Event& event) override {
    ADD_FAILURE() << "Unexpected broadcast " << event.event_name;
  }

  void OnDispatchEventToExtension(const std::string& extension_id,
                                  const extensions::Event& event) override {
    ASSERT_EQ(extension_id, app_id_);

    ASSERT_EQ(event.event_name,
              extensions::api::app_runtime::OnLaunched::kEventName);
    ASSERT_TRUE(event.event_args);
    ASSERT_EQ(1u, event.event_args->GetList().size());

    const base::Value& launch_data = event.event_args->GetList()[0];
    const base::Value* is_kiosk_session =
        launch_data.FindKeyOfType("isKioskSession", base::Value::Type::BOOLEAN);
    ASSERT_TRUE(is_kiosk_session);
    EXPECT_TRUE(is_kiosk_session->GetBool());
    ++kiosk_launch_count_;
  }

 private:
  const std::string app_id_;
  extensions::TestEventRouter* event_router_;
  int kiosk_launch_count_ = 0;

  DISALLOW_COPY_AND_ASSIGN(AppLaunchTracker);
};

class ExternalLoaderHandler {
 public:
  ExternalLoaderHandler(Profile* profile,
                        ExtensionService* extension_service,
                        chromeos::KioskAppExternalLoader* loader)
      : profile_(profile),
        extension_service_(extension_service),
        loader_(loader) {
    loader->set_prefs_interceptor_for_testing(base::BindRepeating(
        &ExternalLoaderHandler::HandlePrefsUpdate, base::Unretained(this)));
    loader->StartLoading();
  }

  ~ExternalLoaderHandler() {
    loader_->set_prefs_interceptor_for_testing(
        chromeos::KioskAppExternalLoader::PrefsInterceptor());
  }

  bool FinishPendingInstall(const extensions::Extension* extension) {
    if (!pending_crx_files_.count(extension->id()) &&
        !pending_update_urls_.count(extension->id())) {
      return false;
    }

    if (!extension_service_->pending_extension_manager()->IsIdPending(
            extension->id())) {
      return false;
    }

    pending_crx_files_.erase(extension->id());
    pending_update_urls_.erase(extension->id());
    extension_service_->pending_extension_manager()->Remove(extension->id());
    extension_service_->AddExtension(extension);
    extensions::InstallTracker::Get(profile_)->OnFinishCrxInstall(
        extension->id(), true);
    return true;
  }

  const std::set<std::string>& pending_crx_files() const {
    return pending_crx_files_;
  }
  const std::set<std::string>& pending_update_urls() const {
    return pending_update_urls_;
  }

 protected:
  void HandlePrefsUpdate(bool initial_load,
                         std::unique_ptr<base::DictionaryValue> prefs) {
    for (const auto& entry : prefs->DictItems()) {
      const base::Value* external_crx = entry.second.FindKeyOfType(
          extensions::ExternalProviderImpl::kExternalCrx,
          base::Value::Type::STRING);
      const base::Value* external_update_url = entry.second.FindKeyOfType(
          extensions::ExternalProviderImpl::kExternalUpdateUrl,
          base::Value::Type::STRING);

      ASSERT_TRUE(external_crx || external_update_url);

      const base::Value* keep_if_present = entry.second.FindKeyOfType(
          extensions::ExternalProviderImpl::kKeepIfPresent,
          base::Value::Type::BOOLEAN);
      if (keep_if_present && keep_if_present->GetBool())
        continue;

      if (external_update_url &&
          extension_service_->GetInstalledExtension(entry.first)) {
        continue;
      }

      const base::Value* version = entry.second.FindKeyOfType(
          extensions::ExternalProviderImpl::kExternalVersion,
          base::Value::Type::STRING);
      const base::Value* install_param = entry.second.FindKeyOfType(
          extensions::ExternalProviderImpl::kInstallParam,
          base::Value::Type::STRING);

      if (external_crx) {
        pending_crx_files_.insert(entry.first);
        extension_service_->pending_extension_manager()->AddFromExternalFile(
            entry.first, extensions::Manifest::EXTERNAL_PREF,
            base::Version(version ? version->GetString() : ""), 0, true);
      } else {
        pending_update_urls_.insert(entry.first);
        extension_service_->pending_extension_manager()
            ->AddFromExternalUpdateUrl(
                entry.first, install_param ? install_param->GetString() : "",
                GURL(external_update_url->GetString()),
                extensions::Manifest::EXTERNAL_PREF, 0, true);
      }
      extensions::InstallTracker::Get(profile_)->OnBeginCrxInstall(entry.first);
    }
  }

 private:
  Profile* profile_;
  ExtensionService* extension_service_;
  chromeos::KioskAppExternalLoader* loader_;

  std::set<std::string> pending_crx_files_;
  std::set<std::string> pending_update_urls_;

  DISALLOW_COPY_AND_ASSIGN(ExternalLoaderHandler);
};

class KioskTestAppBuilder {
 public:
  explicit KioskTestAppBuilder(const std::string& app_id) : app_id_(app_id) {}

  ~KioskTestAppBuilder() = default;

  scoped_refptr<const extensions::Extension> Build() {
    std::unique_ptr<base::DictionaryValue> background =
        DictionaryBuilder()
            .Set("scripts", ListBuilder().Append("background.js").Build())
            .Build();

    DictionaryBuilder manifest_builder;
    manifest_builder.Set("name", "Test kiosk app")
        .Set("version", "1.0")
        .Set("manifest_version", 2)
        .Set("app", DictionaryBuilder()
                        .Set("background", std::move(background))
                        .Build());

    if (kiosk_enabled_)
      manifest_builder.SetBoolean("kiosk_enabled", kiosk_enabled_);
    if (offline_enabled_)
      manifest_builder.SetBoolean("offline_enabled", offline_enabled_);

    if (!secondary_extensions_.empty()) {
      ListBuilder secondary_extension_list_builder;
      for (const auto& secondary_extension : secondary_extensions_) {
        secondary_extension_list_builder.Append(
            DictionaryBuilder().Set("id", secondary_extension).Build());
      }
      manifest_builder.Set("kiosk_secondary_apps",
                           secondary_extension_list_builder.Build());
    }

    return extensions::ExtensionBuilder()
        .SetManifest(manifest_builder.Build())
        .SetID(app_id_)
        .Build();
  }

  void set_kiosk_enabled(bool enabled) { kiosk_enabled_ = enabled; }
  void set_offline_enabled(bool enabled) { offline_enabled_ = enabled; }

  void AddSecondaryExtension(const std::string& extension_id) {
    secondary_extensions_.push_back(extension_id);
  }

 private:
  const std::string app_id_;
  bool kiosk_enabled_ = true;
  bool offline_enabled_ = false;
  std::vector<std::string> secondary_extensions_;

  DISALLOW_COPY_AND_ASSIGN(KioskTestAppBuilder);
};

class TestAppLaunchDelegate : public chromeos::StartupAppLauncher::Delegate {
 public:
  TestAppLaunchDelegate() = default;
  ~TestAppLaunchDelegate() override = default;

  LaunchState launch_state() const { return launch_state_; }
  const std::vector<LaunchState>& launch_state_changes() const {
    return launch_state_changes_;
  }
  chromeos::KioskAppLaunchError::Error launch_error() const {
    return launch_error_;
  }

  void set_network_ready(bool network_ready) { network_ready_ = network_ready; }
  void set_should_skip_app_installation(bool skip_app_installation) {
    should_skip_app_installation_ = skip_app_installation;
  }
  void set_showing_network_config_screen(bool showing) {
    showing_network_config_screen_ = showing;
  }

  void ClearLaunchStateChanges() { launch_state_changes_.clear(); }

  void WaitForLaunchStates(const std::set<LaunchState>& states) {
    if (states.count(launch_state_))
      return;

    base::RunLoop run_loop;
    waiting_for_launch_states_ = states;
    quit_states_waiter_ = run_loop.QuitClosure();
    run_loop.Run();
  }

  // chromeos::StartupAppLauncher::Delegate:
  void InitializeNetwork() override {
    SetLaunchState(LaunchState::kInitializingNetwork);
  }
  bool IsNetworkReady() override { return network_ready_; }
  bool ShouldSkipAppInstallation() override {
    return should_skip_app_installation_;
  }
  void OnInstallingApp() override {
    SetLaunchState(LaunchState::kInstallingApp);
  }
  void OnReadyToLaunch() override {
    SetLaunchState(LaunchState::kReadyToLaunch);
  }
  void OnLaunchSucceeded() override {
    SetLaunchState(LaunchState::kLaunchSucceeded);
  }
  void OnLaunchFailed(chromeos::KioskAppLaunchError::Error error) override {
    launch_error_ = error;
    SetLaunchState(LaunchState::kLaunchFailed);
  }
  bool IsShowingNetworkConfigScreen() override {
    return showing_network_config_screen_;
  }

 private:
  void SetLaunchState(LaunchState state) {
    launch_state_changes_.push_back(state);
    launch_state_ = state;

    if (quit_states_waiter_ && waiting_for_launch_states_.count(state)) {
      waiting_for_launch_states_.clear();
      std::move(quit_states_waiter_).Run();
    }
  }

  LaunchState launch_state_ = LaunchState::kNotStarted;
  std::vector<LaunchState> launch_state_changes_;
  chromeos::KioskAppLaunchError::Error launch_error_ =
      chromeos::KioskAppLaunchError::NONE;

  bool network_ready_ = false;
  bool showing_network_config_screen_ = false;

  bool should_skip_app_installation_ = false;

  base::Closure quit_states_waiter_;
  std::set<LaunchState> waiting_for_launch_states_;

  DISALLOW_COPY_AND_ASSIGN(TestAppLaunchDelegate);
};

}  // namespace

class StartupAppLauncherTest : public testing::Test,
                               public chromeos::KioskAppManager::Overrides {
 public:
  StartupAppLauncherTest() : local_state_(TestingBrowserProcess::GetGlobal()) {}
  ~StartupAppLauncherTest() override = default;

  // testing::Test:
  void SetUp() override {
    command_line_.GetProcessCommandLine()->AppendSwitch(
        switches::kForceAppMode);
    command_line_.GetProcessCommandLine()->AppendSwitch(switches::kAppId);

    chromeos::KioskAppManager::InitializeForTesting(this);

    InitializePrimaryAppState();

    // Initialize the testing profile's extension service,
    extensions::TestExtensionSystem* extension_system =
        static_cast<extensions::TestExtensionSystem*>(
            extensions::ExtensionSystem::Get(&profile_));
    extension_system->CreateExtensionService(
        base::CommandLine::ForCurrentProcess(),
        base::FilePath() /* install_directory */,
        false /* autoupdate_enabled */);

    extensions::TestEventRouter* event_router =
        extensions::CreateAndUseTestEventRouter(&profile_);
    app_launch_tracker_ =
        std::make_unique<AppLaunchTracker>(kTestPrimaryAppId, event_router);

    primary_external_loader_ =
        base::MakeRefCounted<chromeos::KioskAppExternalLoader>(
            chromeos::KioskAppExternalLoader::AppClass::PRIMARY);
    primary_loader_handler_ = std::make_unique<ExternalLoaderHandler>(
        &profile_, extension_system->extension_service(),
        primary_external_loader_.get());

    secondary_external_loader_ =
        base::MakeRefCounted<chromeos::KioskAppExternalLoader>(
            chromeos::KioskAppExternalLoader::AppClass::SECONDARY);
    secondary_loader_handler_ = std::make_unique<ExternalLoaderHandler>(
        &profile_, extension_system->extension_service(),
        secondary_external_loader_.get());

    startup_app_launcher_ = std::make_unique<chromeos::StartupAppLauncher>(
        &profile_, kTestPrimaryAppId, false /*diagnostic_mode*/,
        &startup_launch_delegate_);
  }
  void TearDown() override {
    startup_app_launcher_.reset();
    external_cache_ = nullptr;

    primary_loader_handler_.reset();
    primary_external_loader_ = nullptr;

    secondary_loader_handler_.reset();
    secondary_external_loader_ = nullptr;

    app_launch_tracker_.reset();

    chromeos::KioskAppManager::Shutdown();

    accounts_settings_helper_.RestoreProvider();
  }

  // chromeos::KioskAppManager::Overrides:
  std::unique_ptr<chromeos::ExternalCache> CreateExternalCache(
      chromeos::ExternalCacheDelegate* delegate,
      bool always_check_updates) override {
    auto cache = std::make_unique<chromeos::TestExternalCache>(
        delegate, always_check_updates);
    external_cache_ = cache.get();
    return cache;
  }
  std::unique_ptr<chromeos::AppSession> CreateAppSession() override {
    EXPECT_FALSE(kiosk_app_session_initialized_);
    kiosk_app_session_initialized_ = true;
    return nullptr;
  }

 protected:
  TestAppLaunchDelegate startup_launch_delegate_;

  std::unique_ptr<chromeos::StartupAppLauncher> startup_app_launcher_;
  std::unique_ptr<ExternalLoaderHandler> primary_loader_handler_;
  std::unique_ptr<ExternalLoaderHandler> secondary_loader_handler_;
  std::unique_ptr<AppLaunchTracker> app_launch_tracker_;

  chromeos::TestExternalCache* external_cache_ = nullptr;

  bool kiosk_app_session_initialized_ = false;
  session_manager::SessionManager session_manager_;

 private:
  void InitializePrimaryAppState() {
    chromeos::KioskAppManager::Get()->AddAppForTest(
        kTestPrimaryAppId, AccountId::FromUserEmail("user@test"),
        GURL("http://cws/"), "");

    accounts_settings_helper_.ReplaceProvider(
        chromeos::kAccountsPrefDeviceLocalAccounts);

    auto account = std::make_unique<base::DictionaryValue>();
    account->SetKey(chromeos::kAccountsPrefDeviceLocalAccountsKeyId,
                    base::Value("user@test"));
    account->SetKey(chromeos::kAccountsPrefDeviceLocalAccountsKeyType,
                    base::Value(policy::DeviceLocalAccount::TYPE_KIOSK_APP));
    account->SetKey(chromeos::kAccountsPrefDeviceLocalAccountsKeyKioskAppId,
                    base::Value(kTestPrimaryAppId));
    base::ListValue accounts;
    accounts.Append(std::move(account));

    accounts_settings_helper_.Set(chromeos::kAccountsPrefDeviceLocalAccounts,
                                  accounts);
    accounts_settings_helper_.SetString(
        chromeos::kAccountsPrefDeviceLocalAccountAutoLoginId, "user@test");
    accounts_settings_helper_.SetInteger(
        chromeos::kAccountsPrefDeviceLocalAccountAutoLoginDelay, 0);
  }

  base::test::ScopedCommandLine command_line_;

  ScopedTestingLocalState local_state_;
  content::TestBrowserThreadBundle browser_thread_bundle_;
  chromeos::ScopedCrosSettingsTestHelper accounts_settings_helper_;
  TestingProfile profile_;

  scoped_refptr<chromeos::KioskAppExternalLoader> primary_external_loader_;
  scoped_refptr<chromeos::KioskAppExternalLoader> secondary_external_loader_;

  DISALLOW_COPY_AND_ASSIGN(StartupAppLauncherTest);
};

TEST_F(StartupAppLauncherTest, PrimaryAppLaunchFlow) {
  startup_app_launcher_->Initialize();

  startup_launch_delegate_.WaitForLaunchStates(
      {LaunchState::kInitializingNetwork});

  EXPECT_EQ(LaunchState::kInitializingNetwork,
            startup_launch_delegate_.launch_state());
  EXPECT_EQ(std::vector<LaunchState>({LaunchState::kInitializingNetwork}),
            startup_launch_delegate_.launch_state_changes());
  startup_launch_delegate_.ClearLaunchStateChanges();

  startup_launch_delegate_.set_network_ready(true);
  startup_app_launcher_->ContinueWithNetworkReady();
  EXPECT_TRUE(startup_launch_delegate_.launch_state_changes().empty());
  EXPECT_TRUE(primary_loader_handler_->pending_crx_files().empty());
  EXPECT_TRUE(primary_loader_handler_->pending_update_urls().empty());

  EXPECT_EQ(std::set<std::string>({kTestPrimaryAppId}),
            external_cache_->pending_downloads());
  ASSERT_TRUE(external_cache_->SimulateExtensionDownloadFinished(
      kTestPrimaryAppId, "test_crx_file", "1.0"));

  EXPECT_EQ(std::vector<LaunchState>({LaunchState::kInstallingApp}),
            startup_launch_delegate_.launch_state_changes());

  EXPECT_TRUE(primary_loader_handler_->pending_update_urls().empty());
  EXPECT_EQ(std::set<std::string>({kTestPrimaryAppId}),
            primary_loader_handler_->pending_crx_files());
  startup_launch_delegate_.ClearLaunchStateChanges();

  scoped_refptr<const extensions::Extension> primary_app =
      KioskTestAppBuilder(kTestPrimaryAppId).Build();

  ASSERT_TRUE(primary_loader_handler_->FinishPendingInstall(primary_app.get()));

  startup_launch_delegate_.WaitForLaunchStates({LaunchState::kReadyToLaunch});
  EXPECT_EQ(std::vector<LaunchState>({LaunchState::kReadyToLaunch}),
            startup_launch_delegate_.launch_state_changes());
  startup_launch_delegate_.ClearLaunchStateChanges();

  EXPECT_FALSE(kiosk_app_session_initialized_);
  EXPECT_FALSE(session_manager_.IsSessionStarted());
  startup_app_launcher_->LaunchApp();

  EXPECT_EQ(std::vector<LaunchState>({LaunchState::kLaunchSucceeded}),
            startup_launch_delegate_.launch_state_changes());
  EXPECT_EQ(1, app_launch_tracker_->kiosk_launch_count());

  EXPECT_TRUE(kiosk_app_session_initialized_);
  EXPECT_TRUE(session_manager_.IsSessionStarted());
}

TEST_F(StartupAppLauncherTest, LaunchWithSecondaryApp) {
  startup_app_launcher_->Initialize();

  startup_launch_delegate_.WaitForLaunchStates(
      {LaunchState::kInitializingNetwork});

  EXPECT_EQ(LaunchState::kInitializingNetwork,
            startup_launch_delegate_.launch_state());
  EXPECT_EQ(std::vector<LaunchState>({LaunchState::kInitializingNetwork}),
            startup_launch_delegate_.launch_state_changes());
  startup_launch_delegate_.ClearLaunchStateChanges();

  startup_launch_delegate_.set_network_ready(true);
  startup_app_launcher_->ContinueWithNetworkReady();
  EXPECT_TRUE(startup_launch_delegate_.launch_state_changes().empty());
  EXPECT_TRUE(primary_loader_handler_->pending_crx_files().empty());
  EXPECT_TRUE(primary_loader_handler_->pending_update_urls().empty());

  EXPECT_EQ(std::set<std::string>({kTestPrimaryAppId}),
            external_cache_->pending_downloads());
  ASSERT_TRUE(external_cache_->SimulateExtensionDownloadFinished(
      kTestPrimaryAppId, "test_crx_file", "1.0"));

  EXPECT_EQ(std::vector<LaunchState>({LaunchState::kInstallingApp}),
            startup_launch_delegate_.launch_state_changes());

  EXPECT_TRUE(primary_loader_handler_->pending_update_urls().empty());
  EXPECT_EQ(std::set<std::string>({kTestPrimaryAppId}),
            primary_loader_handler_->pending_crx_files());
  startup_launch_delegate_.ClearLaunchStateChanges();

  KioskTestAppBuilder primary_app_builder(kTestPrimaryAppId);
  primary_app_builder.AddSecondaryExtension("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
  scoped_refptr<const extensions::Extension> primary_app =
      primary_app_builder.Build();

  ASSERT_TRUE(primary_loader_handler_->FinishPendingInstall(primary_app.get()));

  EXPECT_TRUE(primary_loader_handler_->pending_crx_files().empty());
  EXPECT_TRUE(primary_loader_handler_->pending_update_urls().empty());

  EXPECT_TRUE(secondary_loader_handler_->pending_crx_files().empty());
  EXPECT_EQ(std::set<std::string>({"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"}),
            secondary_loader_handler_->pending_update_urls());

  KioskTestAppBuilder secondary_app_builder("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
  secondary_app_builder.set_kiosk_enabled(false);
  scoped_refptr<const extensions::Extension> secondary_app =
      secondary_app_builder.Build();

  EXPECT_EQ(std::vector<LaunchState>({LaunchState::kInstallingApp}),
            startup_launch_delegate_.launch_state_changes());
  startup_launch_delegate_.ClearLaunchStateChanges();

  ASSERT_TRUE(
      secondary_loader_handler_->FinishPendingInstall(secondary_app.get()));

  startup_launch_delegate_.WaitForLaunchStates({LaunchState::kReadyToLaunch});
  EXPECT_EQ(std::vector<LaunchState>({LaunchState::kReadyToLaunch}),
            startup_launch_delegate_.launch_state_changes());
  startup_launch_delegate_.ClearLaunchStateChanges();

  EXPECT_FALSE(kiosk_app_session_initialized_);
  EXPECT_FALSE(session_manager_.IsSessionStarted());
  startup_app_launcher_->LaunchApp();

  EXPECT_EQ(std::vector<LaunchState>({LaunchState::kLaunchSucceeded}),
            startup_launch_delegate_.launch_state_changes());
  EXPECT_EQ(1, app_launch_tracker_->kiosk_launch_count());

  EXPECT_TRUE(kiosk_app_session_initialized_);
  EXPECT_TRUE(session_manager_.IsSessionStarted());
}
