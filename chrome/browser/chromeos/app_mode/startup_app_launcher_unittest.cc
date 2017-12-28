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
#include "chrome/browser/chromeos/settings/scoped_cros_settings_test_helper.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/extensions/extension_service_test_base.h"
#include "chrome/browser/extensions/external_provider_impl.h"
#include "chrome/browser/extensions/install_tracker.h"
#include "chrome/browser/extensions/pending_extension_manager.h"
#include "chrome/common/chrome_switches.h"
#include "chromeos/settings/cros_settings_names.h"
#include "components/session_manager/core/session_manager.h"
#include "components/signin/core/account_id/account_id.h"
#include "extensions/browser/external_install_info.h"
#include "extensions/browser/test_event_router.h"
#include "extensions/common/api/app_runtime.h"
#include "extensions/common/extension.h"
#include "extensions/common/extension_builder.h"
#include "extensions/common/manifest.h"
#include "extensions/common/value_builder.h"
#include "url/gurl.h"

using extensions::DictionaryBuilder;
using extensions::ExtensionBuilder;
using extensions::ExternalInstallInfoFile;
using extensions::ExternalInstallInfoUpdateUrl;
using extensions::ListBuilder;

namespace {

constexpr char kTestPrimaryAppId[] = "abcdefghabcdefghabcdefghabcdefgh";
constexpr char kTestUserAccount[] = "user@test";

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

class TestKioskLoaderVisitor
    : public extensions::ExternalProviderInterface::VisitorInterface {
 public:
  TestKioskLoaderVisitor(content::BrowserContext* browser_context,
                         ExtensionService* extension_service)
      : browser_context_(browser_context),
        extension_service_(extension_service) {}

  ~TestKioskLoaderVisitor() override = default;

  const std::set<std::string>& pending_crx_files() const {
    return pending_crx_files_;
  }
  const std::set<std::string>& pending_update_urls() const {
    return pending_update_urls_;
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
    extensions::InstallTracker::Get(browser_context_)
        ->OnFinishCrxInstall(extension->id(), true);
    return true;
  }

  // extensions::ExternalProviderInterface::VisitorInterface:
  bool OnExternalExtensionFileFound(
      const ExternalInstallInfoFile& info) override {
    const extensions::Extension* existing =
        extension_service_->GetExtensionById(info.extension_id, true);
    // Alredy exists, and does not require update.
    if (existing && existing->version().CompareTo(info.version) >= 0)
      return false;

    if (!extension_service_->pending_extension_manager()->AddFromExternalFile(
            info.extension_id, info.crx_location, info.version,
            info.creation_flags, info.mark_acknowledged)) {
      return false;
    }

    pending_crx_files_.insert(info.extension_id);
    extensions::InstallTracker::Get(browser_context_)
        ->OnBeginCrxInstall(info.extension_id);
    return true;
  }
  bool OnExternalExtensionUpdateUrlFound(
      const ExternalInstallInfoUpdateUrl& info,
      bool is_initial_load) override {
    if (extension_service_->GetExtensionById(info.extension_id, true))
      return false;

    if (!extension_service_->pending_extension_manager()
             ->AddFromExternalUpdateUrl(
                 info.extension_id, info.install_parameter, info.update_url,
                 info.download_location, info.creation_flags,
                 info.mark_acknowledged)) {
      return false;
    }

    pending_update_urls_.insert(info.extension_id);
    extensions::InstallTracker::Get(browser_context_)
        ->OnBeginCrxInstall(info.extension_id);
    return true;
  }
  void OnExternalProviderReady(
      const extensions::ExternalProviderInterface* provider) override {}
  void OnExternalProviderUpdateComplete(
      const extensions::ExternalProviderInterface* provider,
      const std::vector<ExternalInstallInfoUpdateUrl>& update_url_extensions,
      const std::vector<ExternalInstallInfoFile>& file_extensions,
      const std::set<std::string>& removed_extensions) override {
    for (const auto& extension : update_url_extensions)
      OnExternalExtensionUpdateUrlFound(extension, false);

    for (const auto& extension : file_extensions)
      OnExternalExtensionFileFound(extension);

    for (const auto& extension_id : removed_extensions) {
      extension_service_->UninstallExtension(
          extension_id,
          extensions::UNINSTALL_REASON_ORPHANED_EXTERNAL_EXTENSION, nullptr);
    }
  }

 private:
  content::BrowserContext* const browser_context_;
  ExtensionService* const extension_service_;

  std::set<std::string> pending_crx_files_;
  std::set<std::string> pending_update_urls_;

  DISALLOW_COPY_AND_ASSIGN(TestKioskLoaderVisitor);
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

class StartupAppLauncherTest : public extensions::ExtensionServiceTestBase,
                               public chromeos::KioskAppManager::Overrides {
 public:
  StartupAppLauncherTest() = default;
  ~StartupAppLauncherTest() override = default;

  // testing::Test:
  void SetUp() override {
    command_line_.GetProcessCommandLine()->AppendSwitch(
        switches::kForceAppMode);
    command_line_.GetProcessCommandLine()->AppendSwitch(switches::kAppId);

    chromeos::KioskAppManager::InitializeForTesting(this);

    InitializePrimaryAppState();

    extensions::ExtensionServiceTestBase::SetUp();

    InitializeEmptyExtensionService();
    external_apps_loader_handler_ =
        std::make_unique<TestKioskLoaderVisitor>(browser_context(), service());
    InitializeKioskAppsProviders(external_apps_loader_handler_.get());

    extensions::TestEventRouter* event_router =
        extensions::CreateAndUseTestEventRouter(browser_context());
    app_launch_tracker_ =
        std::make_unique<AppLaunchTracker>(kTestPrimaryAppId, event_router);

    startup_app_launcher_ = std::make_unique<chromeos::StartupAppLauncher>(
        profile(), kTestPrimaryAppId, false /*diagnostic_mode*/,
        &startup_launch_delegate_);
  }

  void TearDown() override {
    startup_app_launcher_.reset();
    external_cache_ = nullptr;

    primary_app_provider_->ServiceShutdown();
    secondary_apps_provider_->ServiceShutdown();
    external_apps_loader_handler_.reset();

    app_launch_tracker_.reset();

    chromeos::KioskAppManager::Shutdown();

    accounts_settings_helper_.reset();

    extensions::ExtensionServiceTestBase::TearDown();
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
  std::unique_ptr<AppLaunchTracker> app_launch_tracker_;
  std::unique_ptr<TestKioskLoaderVisitor> external_apps_loader_handler_;

  chromeos::TestExternalCache* external_cache_ = nullptr;

  bool kiosk_app_session_initialized_ = false;
  session_manager::SessionManager session_manager_;

 private:
  void InitializePrimaryAppState() {
    chromeos::KioskAppManager::Get()->AddAppForTest(
        kTestPrimaryAppId, AccountId::FromUserEmail(kTestUserAccount),
        GURL("http://cws/"), "");

    accounts_settings_helper_ =
        std::make_unique<chromeos::ScopedCrosSettingsTestHelper>(
            false /*create_service*/);
    accounts_settings_helper_->ReplaceProvider(
        chromeos::kAccountsPrefDeviceLocalAccounts);

    auto account = std::make_unique<base::DictionaryValue>();
    account->SetKey(chromeos::kAccountsPrefDeviceLocalAccountsKeyId,
                    base::Value(kTestUserAccount));
    account->SetKey(chromeos::kAccountsPrefDeviceLocalAccountsKeyType,
                    base::Value(policy::DeviceLocalAccount::TYPE_KIOSK_APP));
    account->SetKey(chromeos::kAccountsPrefDeviceLocalAccountsKeyKioskAppId,
                    base::Value(kTestPrimaryAppId));
    base::ListValue accounts;
    accounts.Append(std::move(account));

    accounts_settings_helper_->Set(chromeos::kAccountsPrefDeviceLocalAccounts,
                                   accounts);
    accounts_settings_helper_->SetString(
        chromeos::kAccountsPrefDeviceLocalAccountAutoLoginId, kTestUserAccount);
    accounts_settings_helper_->SetInteger(
        chromeos::kAccountsPrefDeviceLocalAccountAutoLoginDelay, 0);
  }

  void InitializeKioskAppsProviders(TestKioskLoaderVisitor* visitor) {
    primary_app_provider_ = std::make_unique<extensions::ExternalProviderImpl>(
        visitor,
        base::MakeRefCounted<chromeos::KioskAppExternalLoader>(
            chromeos::KioskAppExternalLoader::AppClass::PRIMARY),
        profile(), extensions::Manifest::EXTERNAL_POLICY,
        extensions::Manifest::INVALID_LOCATION,
        extensions::Extension::NO_FLAGS);
    primary_app_provider_->set_auto_acknowledge(true);
    primary_app_provider_->set_install_immediately(true);
    primary_app_provider_->set_allow_updates(true);
    primary_app_provider_->VisitRegisteredExtension();

    secondary_apps_provider_ =
        std::make_unique<extensions::ExternalProviderImpl>(
            visitor,
            base::MakeRefCounted<chromeos::KioskAppExternalLoader>(
                chromeos::KioskAppExternalLoader::AppClass::SECONDARY),
            profile(), extensions::Manifest::EXTERNAL_PREF,
            extensions::Manifest::EXTERNAL_PREF_DOWNLOAD,
            extensions::Extension::NO_FLAGS);
    secondary_apps_provider_->set_auto_acknowledge(true);
    secondary_apps_provider_->set_install_immediately(true);
    secondary_apps_provider_->set_allow_updates(true);
    secondary_apps_provider_->VisitRegisteredExtension();
  }

  base::test::ScopedCommandLine command_line_;

  std::unique_ptr<chromeos::ScopedCrosSettingsTestHelper>
      accounts_settings_helper_;

  std::unique_ptr<extensions::ExternalProviderImpl> primary_app_provider_;
  std::unique_ptr<extensions::ExternalProviderImpl> secondary_apps_provider_;

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
  EXPECT_TRUE(external_apps_loader_handler_->pending_crx_files().empty());
  EXPECT_TRUE(external_apps_loader_handler_->pending_update_urls().empty());

  ASSERT_TRUE(external_cache_);
  EXPECT_EQ(std::set<std::string>({kTestPrimaryAppId}),
            external_cache_->pending_downloads());
  ASSERT_TRUE(external_cache_->SimulateExtensionDownloadFinished(
      kTestPrimaryAppId,
      temp_dir().GetPath().AppendASCII("test_crx_file").value(), "1.0"));

  EXPECT_EQ(std::vector<LaunchState>({LaunchState::kInstallingApp}),
            startup_launch_delegate_.launch_state_changes());

  EXPECT_TRUE(external_apps_loader_handler_->pending_update_urls().empty());
  EXPECT_EQ(std::set<std::string>({kTestPrimaryAppId}),
            external_apps_loader_handler_->pending_crx_files());
  startup_launch_delegate_.ClearLaunchStateChanges();

  scoped_refptr<const extensions::Extension> primary_app =
      KioskTestAppBuilder(kTestPrimaryAppId).Build();

  ASSERT_TRUE(
      external_apps_loader_handler_->FinishPendingInstall(primary_app.get()));

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
  EXPECT_TRUE(external_apps_loader_handler_->pending_crx_files().empty());
  EXPECT_TRUE(external_apps_loader_handler_->pending_update_urls().empty());

  EXPECT_EQ(std::set<std::string>({kTestPrimaryAppId}),
            external_cache_->pending_downloads());
  ASSERT_TRUE(external_cache_->SimulateExtensionDownloadFinished(
      kTestPrimaryAppId,
      temp_dir().GetPath().AppendASCII("test_crx_file").value(), "1.0"));

  EXPECT_EQ(std::vector<LaunchState>({LaunchState::kInstallingApp}),
            startup_launch_delegate_.launch_state_changes());

  EXPECT_TRUE(external_apps_loader_handler_->pending_update_urls().empty());
  EXPECT_EQ(std::set<std::string>({kTestPrimaryAppId}),
            external_apps_loader_handler_->pending_crx_files());
  startup_launch_delegate_.ClearLaunchStateChanges();

  KioskTestAppBuilder primary_app_builder(kTestPrimaryAppId);
  primary_app_builder.AddSecondaryExtension("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
  scoped_refptr<const extensions::Extension> primary_app =
      primary_app_builder.Build();

  ASSERT_TRUE(
      external_apps_loader_handler_->FinishPendingInstall(primary_app.get()));

  EXPECT_TRUE(external_apps_loader_handler_->pending_crx_files().empty());
  EXPECT_EQ(std::set<std::string>({"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"}),
            external_apps_loader_handler_->pending_update_urls());

  KioskTestAppBuilder secondary_app_builder("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
  secondary_app_builder.set_kiosk_enabled(false);
  scoped_refptr<const extensions::Extension> secondary_app =
      secondary_app_builder.Build();

  EXPECT_EQ(std::vector<LaunchState>({LaunchState::kInstallingApp}),
            startup_launch_delegate_.launch_state_changes());
  startup_launch_delegate_.ClearLaunchStateChanges();

  ASSERT_TRUE(
      external_apps_loader_handler_->FinishPendingInstall(secondary_app.get()));

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
