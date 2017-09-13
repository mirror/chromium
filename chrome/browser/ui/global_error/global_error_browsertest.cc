// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/outdated_upgrade_bubble_view.h"

#include "build/build_config.h"
#include "chrome/browser/extensions/extension_disabled_ui.h"
#include "chrome/browser/extensions/extension_error_ui_default.h"
#include "chrome/browser/extensions/external_install_error.h"
#include "chrome/browser/recovery/recovery_install_global_error.h"
#include "chrome/browser/signin/signin_global_error.h"
#include "chrome/browser/ui/test/test_browser_dialog.h"

#include "base/json/json_file_value_serializer.h"
#include "base/path_service.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/extensions/extension_error_controller.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/extensions/extension_service_test_with_install.h"
#include "chrome/browser/extensions/test_blacklist.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/global_error/global_error_service.h"
#include "chrome/browser/ui/global_error/global_error_service_factory.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/extensions/extension_constants.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/browser/notification_service.h"
#include "content/public/test/test_utils.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_system.h"
#include "extensions/browser/mock_external_provider.h"
#include "extensions/browser/pref_names.h"
#include "extensions/common/feature_switch.h"

#if defined(OS_WIN)
#include "chrome/browser/safe_browsing/chrome_cleaner/srt_global_error_win.h"
#endif

namespace {
bool IsGlobalErrorWithBubble(const content::NotificationSource& source,
                             const content::NotificationDetails& details) {
  GlobalError* error = content::Details<GlobalError>(details).ptr();
  DLOG(INFO) << error->MenuItemLabel();
  return error->HasBubbleView();
}
}  // namespace

// Non-bubbles:
// ExternalInstallMenuAlert (external_install_error.cc)
// ErrorBadge (warning_badge_service.cc)
class GlobalErrorBubbleTest : public DialogBrowserTest {
 public:
  GlobalErrorBubbleTest() = default;

  // DialogBrowserTest:
  bool SetUpUserDataDirectory() override;
  void ShowDialog(const std::string& name) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(GlobalErrorBubbleTest);
};

bool GlobalErrorBubbleTest::SetUpUserDataDirectory() {
  // Before creating a Profile, ensure ExtensionPrefs::SetAlertSystemFirstRun()
  // will return true by triggering its fuse in profile preferences. Otherwise,
  // most extension error bubbles are suppressed.
  base::FilePath profile_dir;
  CHECK(PathService::Get(chrome::DIR_USER_DATA, &profile_dir));
  profile_dir = profile_dir.AppendASCII(TestingProfile::kTestUserProfileDir);
  CHECK(base::CreateDirectory(profile_dir));
  base::DictionaryValue prefs_dict;
  prefs_dict.SetBoolean(extensions::pref_names::kAlertsInitialized, true);
  CHECK(JSONFileValueSerializer(
            profile_dir.AppendASCII(chrome::kPreferencesFilename))
            .Serialize(prefs_dict));
  return DialogBrowserTest::SetUpUserDataDirectory();
}

void GlobalErrorBubbleTest::ShowDialog(const std::string& name) {
  content::WindowedNotificationObserver global_errors_updated(
      chrome::NOTIFICATION_GLOBAL_ERRORS_CHANGED,
      base::Bind(&IsGlobalErrorWithBubble));
  Profile* profile = browser()->profile();
  ExtensionService* extension_service =
      extensions::ExtensionSystem::Get(profile)->extension_service();
  extensions::ExtensionRegistry* extension_registry =
      extensions::ExtensionRegistry::Get(profile);
  const extensions::Extension* test_extension =
      extension_registry->GetInstalledExtension(extensions::kWebStoreAppId);

  base::FilePath extensions_test_data_dir;
  if (!PathService::Get(chrome::DIR_TEST_DATA, &extensions_test_data_dir)) {
    ADD_FAILURE();
    return;
  }
  extensions_test_data_dir = extensions_test_data_dir.AppendASCII("extensions");

  // Some need a temp dir, which must exist while waiting for the install to
  // complete. IO is needed to delete the temo dir.
  base::ThreadRestrictions::ScopedAllowIO allow_io;
  base::ScopedTempDir temp_dir;

  extensions::TestBlacklist test_blacklist(extensions::Blacklist::Get(profile));
  bool async = true;
  bool needs_show = true;
  if (name == "ExtensionDisabledGlobalError") {
    extensions::AddExtensionDisabledError(extension_service, test_extension,
                                          false);
    async = true;
  } else if (name == "ExtensionDisabledGlobalErrorRemote") {
    extensions::AddExtensionDisabledError(extension_service, test_extension,
                                          true);
    async = true;
  } else if (name == "ExtensionGlobalError") {
    extension_registry->AddBlacklisted(test_extension);
    // Only BLACKLISTED_MALWARE seems to do anything interesting. Other types
    // are greylisted, not blacklisted.
    test_blacklist.SetBlacklistState(extensions::kWebStoreAppId,
                                     extensions::BLACKLISTED_MALWARE, true);
    // Ensure ExtensionService::ManageBlacklist() runs, which shows the dialog.
    // (This flow doesn't use NOTIFICATION_GLOBAL_ERRORS_CHANGED). This is
    // asynchronous, and using TestBlacklist ensures the tasks run without
    // delay, but some tasks run on the IO thread, so post a task there to
    // ensure it was flushed.
    static_cast<extensions::Blacklist::Observer*>(extension_service)
        ->OnBlacklistUpdated();
    base::RunLoop().RunUntilIdle();
    base::RunLoop flush_io;
    content::BrowserThread::PostTaskAndReply(
        content::BrowserThread::IO, FROM_HERE, base::BindOnce(&base::DoNothing),
        flush_io.QuitClosure());
    flush_io.Run();
    base::RunLoop().RunUntilIdle();
    async = false;
    needs_show = false;
  } else if (name == "ExternalInstallBubbleAlert") {
    // To trigger a bubble alert (rather than a menu alert), the extension must
    // come from the webstore, which needs the update to come from a signed crx.
    const char kWebstoreExtension[] = "akjooamlhcgeopfifcmlggaebeocgokj";
    EXPECT_TRUE(temp_dir.CreateUniqueTempDir());
    base::FilePath crx_path = temp_dir.GetPath().AppendASCII("temp.crx");
    extensions::ExtensionServiceTestWithInstall::PackCRX(
        extensions_test_data_dir.AppendASCII("update_from_webstore"),
        extensions_test_data_dir.AppendASCII("update_from_webstore.pem"),
        crx_path);
    auto provider = base::MakeUnique<extensions::MockExternalProvider>(
        extension_service, extensions::Manifest::EXTERNAL_PREF);
    extensions::MockExternalProvider* provider_ptr = provider.get();
    extension_service->AddProviderForTesting(std::move(provider));
    provider_ptr->UpdateOrAddExtension(kWebstoreExtension, "1.0.0.0", crx_path);
    extension_service->CheckForExternalUpdates();

    // ExternalInstallError::OnDialogReady() adds the error and shows the dialog
    // immediately.
    needs_show = false;
  } else if (name == "RecoveryInstallGlobalError") {
    NOTREACHED();
  } else if (name == "SigninGlobalError") {
    NOTREACHED();
  } else if (name == "SRTGlobalError") {
    NOTREACHED();
  } else {
    ADD_FAILURE();
  }

  if (async) {
    global_errors_updated.Wait();
    if (needs_show) {
      GlobalErrorService* service =
          GlobalErrorServiceFactory::GetForProfile(profile);
      GlobalError* error = service->GetFirstGlobalErrorWithBubbleView();
      ASSERT_TRUE(error);
      error->ShowBubbleView(browser());
    }
  }
}

IN_PROC_BROWSER_TEST_F(GlobalErrorBubbleTest,
                       InvokeDialog_ExtensionDisabledGlobalError) {
  RunDialog();
}

IN_PROC_BROWSER_TEST_F(GlobalErrorBubbleTest,
                       InvokeDialog_ExtensionDisabledGlobalErrorRemote) {
  RunDialog();
}

// This shows a non-persistent dialog during a RunLoop::RunUntilIdle(), so it's
// not possible to ensure an event that dismisses the dialog is also processed.
// Disable by default to prevent flakiness in browser_tests.
IN_PROC_BROWSER_TEST_F(GlobalErrorBubbleTest,
                       InvokeDialog_ExtensionGlobalError) {
  RunDialog();
}

IN_PROC_BROWSER_TEST_F(GlobalErrorBubbleTest,
                       InvokeDialog_ExternalInstallBubbleAlert) {
  extensions::FeatureSwitch::ScopedOverride prompt(
      extensions::FeatureSwitch::prompt_for_external_extensions(), true);
  RunDialog();
}

IN_PROC_BROWSER_TEST_F(GlobalErrorBubbleTest,
                       InvokeDialog_RecoveryInstallGlobalError) {
  RunDialog();
}

IN_PROC_BROWSER_TEST_F(GlobalErrorBubbleTest, InvokeDialog_SigninGlobalError) {
  RunDialog();
}

#if defined(OS_WIN)
IN_PROC_BROWSER_TEST_F(GlobalErrorBubbleTest, InvokeDialog_SRTGlobalError) {
  RunDialog();
}
#endif
