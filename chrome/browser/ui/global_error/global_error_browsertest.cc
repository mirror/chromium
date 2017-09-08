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

#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/extensions/extension_error_controller.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/extensions/test_blacklist.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/global_error/global_error_service.h"
#include "chrome/browser/ui/global_error/global_error_service_factory.h"
#include "chrome/common/extensions/extension_constants.h"
#include "content/public/browser/notification_service.h"
#include "content/public/test/test_utils.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_system.h"

#if defined(OS_WIN)
#include "chrome/browser/safe_browsing/chrome_cleaner/srt_global_error_win.h"
#endif

// Non-bubbles:
// ExternalInstallMenuAlert (external_install_error.cc)
// ErrorBadge (warning_badge_service.cc)
class GlobalErrorBubbleTest : public DialogBrowserTest {
 public:
  GlobalErrorBubbleTest() = default;

  // DialogBrowserTest:
  void ShowDialog(const std::string& name) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(GlobalErrorBubbleTest);
};

void GlobalErrorBubbleTest::ShowDialog(const std::string& name) {
  content::WindowedNotificationObserver global_errors_updated(
      chrome::NOTIFICATION_GLOBAL_ERRORS_CHANGED,
      content::NotificationService::AllSources());
  Profile* profile = browser()->profile();
  ExtensionService* extension_service =
      extensions::ExtensionSystem::Get(profile)->extension_service();
  extensions::ExtensionRegistry* extension_registry =
      extensions::ExtensionRegistry::Get(profile);
  const extensions::Extension* test_extension =
      extension_registry->GetInstalledExtension(extensions::kWebStoreAppId);

  extensions::TestBlacklist test_blacklist(extensions::Blacklist::Get(profile));

#if 0
  BLACKLISTED_MALWARE = 1,
  BLACKLISTED_SECURITY_VULNERABILITY = 2,
  BLACKLISTED_CWS_POLICY_VIOLATION = 3,
  BLACKLISTED_POTENTIALLY_UNWANTED = 4,
  BLACKLISTED_UNKNOWN
#endif
  bool async = false;
  if (name == "ExtensionDisabledGlobalError") {
    extensions::AddExtensionDisabledError(extension_service, test_extension,
                                          false);
    async = true;
  } else if (name == "ExtensionDisabledGlobalErrorRemote") {
    extensions::AddExtensionDisabledError(extension_service, test_extension,
                                          true);
    async = true;
  } else if (name == "ExtensionGlobalError") {
    extensions::ExtensionErrorController::EnableAlertsForTesting();
    extension_registry->AddBlacklisted(test_extension);
    test_blacklist.SetBlacklistState(extensions::kWebStoreAppId,
                                     extensions::BLACKLISTED_MALWARE, true);
    static_cast<extensions::Blacklist::Observer*>(extension_service)
        ->OnBlacklistUpdated();
    // Ensure ExtensionService::ManageBlacklist() runs, which shows the dialog.
    base::RunLoop().RunUntilIdle();
  } else if (name == "ExternalInstallBubbleAlert") {
    NOTREACHED();
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
    GlobalErrorService* service =
        GlobalErrorServiceFactory::GetForProfile(profile);
    GlobalError* error = service->GetFirstGlobalErrorWithBubbleView();
    ASSERT_TRUE(error);
    error->ShowBubbleView(browser());
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

IN_PROC_BROWSER_TEST_F(GlobalErrorBubbleTest,
                       InvokeDialog_ExtensionGlobalError) {
  RunDialog();
}

IN_PROC_BROWSER_TEST_F(GlobalErrorBubbleTest,
                       InvokeDialog_ExternalInstallBubbleAlert) {
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
