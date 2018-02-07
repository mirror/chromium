// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/app_list/arc/arc_app_dialog.h"

#include "base/command_line.h"
#include "base/macros.h"
#include "base/run_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/chromeos/arc/arc_session_manager.h"
#include "chrome/browser/chromeos/arc/arc_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/app_list/app_list_controller_delegate.h"
#include "chrome/browser/ui/app_list/app_list_service.h"
#include "chrome/browser/ui/app_list/arc/arc_app_list_prefs.h"
#include "chrome/browser/ui/app_list/arc/arc_app_list_prefs_factory.h"
#include "chrome/browser/ui/app_list/arc/arc_app_utils.h"
#include "chrome/browser/ui/app_list/arc/arc_usb_host_permission_manager.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/arc/arc_util.h"
#include "components/arc/common/app.mojom.h"
#include "components/arc/test/connection_holder_util.h"
#include "components/arc/test/fake_app_instance.h"
#include "content/public/test/test_utils.h"

namespace arc {

class ArcAppDialogViewBrowserTest : public InProcessBrowserTest {
 public:
  ArcAppDialogViewBrowserTest() : weak_ptr_factory_(this) {}

  // InProcessBrowserTest:
  ~ArcAppDialogViewBrowserTest() override {}

  void SetUpCommandLine(base::CommandLine* command_line) override {
    arc::SetArcAvailableCommandLineForTesting(command_line);
  }

  void SetUpInProcessBrowserTestFixture() override {
    ArcSessionManager::DisableUIForTesting();
  }

  void SetUpOnMainThread() override {
    profile_ = browser()->profile();
    arc_app_list_pref_ = ArcAppListPrefs::Get(profile_);
    if (!arc_app_list_pref_) {
      ArcAppListPrefsFactory::GetInstance()->RecreateServiceInstanceForTesting(
          profile_);
    }

    arc::SetArcPlayStoreEnabledForProfile(profile_, true);

    arc_app_list_pref_ = ArcAppListPrefs::Get(profile_);
    DCHECK(arc_app_list_pref_);

    base::RunLoop run_loop;
    arc_app_list_pref_->SetDefaltAppsReadyCallback(run_loop.QuitClosure());
    run_loop.Run();

    app_instance_.reset(new arc::FakeAppInstance(arc_app_list_pref_));
    arc_app_list_pref_->app_connection_holder()->SetInstance(
        app_instance_.get());
    WaitForInstanceReady(arc_app_list_pref_->app_connection_holder());

    // In this setup, we have one app and one shortcut which share one package.
    mojom::AppInfo app;
    app.name = base::StringPrintf("Fake App %d", 0);
    app.package_name = base::StringPrintf("fake.package.%d", 0);
    app.activity = base::StringPrintf("fake.app.%d.activity", 0);
    app.sticky = false;
    app_instance_->SendRefreshAppList(std::vector<mojom::AppInfo>(1, app));

    mojom::ShortcutInfo shortcut;
    shortcut.name = base::StringPrintf("Fake Shortcut %d", 0);
    shortcut.package_name = base::StringPrintf("fake.package.%d", 0);
    shortcut.intent_uri = base::StringPrintf("Fake Shortcut uri %d", 0);
    app_instance_->SendInstallShortcut(shortcut);

    mojom::ArcPackageInfo package;
    package.package_name = base::StringPrintf("fake.package.%d", 0);
    package.package_version = 0;
    package.last_backup_android_id = 0;
    package.last_backup_time = 0;
    package.sync = false;
    app_instance_->SendRefreshPackageList(
        std::vector<mojom::ArcPackageInfo>(1, package));
  }

  void TearDownOnMainThread() override {
    arc_app_list_pref_->app_connection_holder()->CloseInstance(
        app_instance_.get());
    app_instance_.reset();
    ArcSessionManager::Get()->Shutdown();
  }

  void InstallExtraPackage(int extra) {
    mojom::AppInfo app;
    app.name = base::StringPrintf("Fake App %d", extra);
    app.package_name = base::StringPrintf("fake.package.%d", extra);
    app.activity = base::StringPrintf("fake.app.%d.activity", extra);
    app.sticky = false;
    app_instance_->SendAppAdded(app);

    mojom::ArcPackageInfo package;
    package.package_name = base::StringPrintf("fake.package.%d", extra);
    package.package_version = extra;
    package.last_backup_android_id = extra;
    package.last_backup_time = 0;
    package.sync = false;
    app_instance_->SendPackageAdded(package);
  }

  // Ensures the ArcAppDialogView is destoryed.
  void TearDown() override { ASSERT_FALSE(IsArcAppDialogViewAliveForTest()); }

  ArcAppListPrefs* arc_app_list_pref() { return arc_app_list_pref_; }

  FakeAppInstance* instance() { return app_instance_.get(); }

  Profile* profile() { return profile_; }

  void ConfirmCallback(bool accept) { accept_ = accept; }

  bool IsAccepted() const { return accept_; }

  void resetPermissionResult() { accept_ = false; }

  base::WeakPtr<ArcAppDialogViewBrowserTest> weak_ptr() {
    return weak_ptr_factory_.GetWeakPtr();
  }

 private:
  ArcAppListPrefs* arc_app_list_pref_ = nullptr;

  Profile* profile_ = nullptr;

  bool accept_ = false;

  std::unique_ptr<arc::FakeAppInstance> app_instance_;

  base::WeakPtrFactory<ArcAppDialogViewBrowserTest> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(ArcAppDialogViewBrowserTest);
};

// Basic flow of requesting scan device list or access permission.
IN_PROC_BROWSER_TEST_F(ArcAppDialogViewBrowserTest, ArcUsbPermissionBasicFlow) {
  ArcUsbHostPermissionManager* arc_usb_permission_manager =
      ArcUsbHostPermissionManager::GetForBrowserContext(profile());
  DCHECK(arc_usb_permission_manager);

  // Invalid package name. Requesut is automatically rejected.
  const std::string invalid_package = "invalid_package";
  arc_usb_permission_manager->RequestUsbScanDeviceListPermission(
      invalid_package,
      base::BindOnce(&ArcAppDialogViewBrowserTest::ConfirmCallback,
                     weak_ptr()));
  EXPECT_FALSE(IsArcAppDialogViewAliveForTest());
  EXPECT_FALSE(IsAccepted());

  const std::string package_name = base::StringPrintf("fake.package.%d", 0);
  const std::string guid = "TestGuidXXXXXX";
  const base::string16 device_name = base::UTF8ToUTF16("TestDeviceName");
  const base::string16 serial_number = base::UTF8ToUTF16("TestSerialNumber");
  uint16_t vendor_id = 123;
  uint16_t product_id = 456;

  // Package sends scan devicelist request.
  arc_usb_permission_manager->RequestUsbScanDeviceListPermission(
      package_name,
      base::BindOnce(&ArcAppDialogViewBrowserTest::ConfirmCallback,
                     weak_ptr()));
  // Dialog is shown. Call runs with false.
  EXPECT_TRUE(IsArcAppDialogViewAliveForTest());
  // Accept the dialog.
  EXPECT_TRUE(CloseAppDialogViewAndConfirmForTest(true));
  content::RunAllPendingInMessageLoop();
  EXPECT_FALSE(IsArcAppDialogViewAliveForTest());
  // Result will apply when next time the package tries to request the
  // permisson.
  EXPECT_FALSE(IsAccepted());
  // Package tries to request scan device list permission again.
  arc_usb_permission_manager->RequestUsbScanDeviceListPermission(
      package_name,
      base::BindOnce(&ArcAppDialogViewBrowserTest::ConfirmCallback,
                     weak_ptr()));
  // Dialog is not shown. Permission is applied.
  EXPECT_FALSE(IsArcAppDialogViewAliveForTest());
  EXPECT_TRUE(IsAccepted());

  resetPermissionResult();
  EXPECT_FALSE(IsAccepted());

  // Package sends device access request.
  arc_usb_permission_manager->RequestUsbAccessPermission(
      package_name, guid, device_name, serial_number, vendor_id, product_id,
      base::BindOnce(&ArcAppDialogViewBrowserTest::ConfirmCallback,
                     weak_ptr()));
  // Dialog is shown.
  EXPECT_TRUE(IsArcAppDialogViewAliveForTest());
  // Accept the dialog.
  EXPECT_TRUE(CloseAppDialogViewAndConfirmForTest(true));
  content::RunAllPendingInMessageLoop();
  EXPECT_FALSE(IsArcAppDialogViewAliveForTest());
  // Permission applies.
  EXPECT_TRUE(IsAccepted());
  // Package sends same device access request again.
  arc_usb_permission_manager->RequestUsbAccessPermission(
      package_name, guid, device_name, serial_number, vendor_id, product_id,
      base::BindOnce(&ArcAppDialogViewBrowserTest::ConfirmCallback,
                     weak_ptr()));
  // Dialog is not shown. Permission still applies.
  EXPECT_FALSE(IsArcAppDialogViewAliveForTest());
  EXPECT_TRUE(IsAccepted());
}

// Multiple requests are sent at same time and are processed in request order.
// Pre accepted request will be auto accepted.
IN_PROC_BROWSER_TEST_F(ArcAppDialogViewBrowserTest,
                       ArcUsbPermissionMultipleRequestFlow) {
  ArcUsbHostPermissionManager* arc_usb_permission_manager =
      ArcUsbHostPermissionManager::GetForBrowserContext(profile());
  DCHECK(arc_usb_permission_manager);

  InstallExtraPackage(1);
  InstallExtraPackage(2);

  const std::string package0 = base::StringPrintf("fake.package.%d", 0);
  const std::string package1 = base::StringPrintf("fake.package.%d", 1);
  const std::string package2 = base::StringPrintf("fake.package.%d", 2);
  const std::string guid = "TestGuidXXXXXX";
  const base::string16 device_name = base::UTF8ToUTF16("TestDeviceName");
  const base::string16 serial_number = base::UTF8ToUTF16("TestSerialNumber");
  uint16_t vendor_id = 123;
  uint16_t product_id = 456;

  // Package0 sends device access request.
  arc_usb_permission_manager->RequestUsbAccessPermission(
      package0, guid, device_name, serial_number, vendor_id, product_id,
      base::BindOnce(&ArcAppDialogViewBrowserTest::ConfirmCallback,
                     weak_ptr()));

  // Package1 sends device access request.
  arc_usb_permission_manager->RequestUsbAccessPermission(
      package1, guid, device_name, serial_number, vendor_id, product_id,
      base::BindOnce(&ArcAppDialogViewBrowserTest::ConfirmCallback,
                     weak_ptr()));

  // Package0 sends device access request again.
  arc_usb_permission_manager->RequestUsbAccessPermission(
      package0, guid, device_name, serial_number, vendor_id, product_id,
      base::BindOnce(&ArcAppDialogViewBrowserTest::ConfirmCallback,
                     weak_ptr()));

  // Package2 sends device access request.
  arc_usb_permission_manager->RequestUsbAccessPermission(
      package2, guid, device_name, serial_number, vendor_id, product_id,
      base::BindOnce(&ArcAppDialogViewBrowserTest::ConfirmCallback,
                     weak_ptr()));

  // Dialog is shown.
  EXPECT_TRUE(IsArcAppDialogViewAliveForTest());
  const auto& pending_requests0 =
      arc_usb_permission_manager->GetPendingRequestsForTest();
  EXPECT_EQ(4u, pending_requests0.size());
  EXPECT_EQ(package0, pending_requests0[0]->package_name);
  EXPECT_EQ(package1, pending_requests0[1]->package_name);
  EXPECT_EQ(package0, pending_requests0[2]->package_name);
  EXPECT_EQ(package2, pending_requests0[3]->package_name);

  // Accept the dialog.
  EXPECT_TRUE(CloseAppDialogViewAndConfirmForTest(true));
  content::RunAllPendingInMessageLoop();

  // Dialog is shown for the next request.
  EXPECT_TRUE(IsArcAppDialogViewAliveForTest());
  const auto& pending_requests1 =
      arc_usb_permission_manager->GetPendingRequestsForTest();
  EXPECT_EQ(3u, pending_requests1.size());
  EXPECT_EQ(package1, pending_requests0[0]->package_name);
  EXPECT_EQ(package0, pending_requests0[1]->package_name);
  EXPECT_EQ(package2, pending_requests0[2]->package_name);

  // Accept the dialog.
  EXPECT_TRUE(CloseAppDialogViewAndConfirmForTest(true));
  content::RunAllPendingInMessageLoop();

  // The 3rd request is the same to the first request so it's automatically
  // confirmed. Dialog is shown for the final request.
  EXPECT_TRUE(IsArcAppDialogViewAliveForTest());
  const auto& pending_requests2 =
      arc_usb_permission_manager->GetPendingRequestsForTest();
  EXPECT_EQ(1u, pending_requests2.size());
  EXPECT_EQ(package2, pending_requests0[0]->package_name);

  // Reject the dialog.
  EXPECT_TRUE(CloseAppDialogViewAndConfirmForTest(false));
  content::RunAllPendingInMessageLoop();

  // All requests are handled. No dialog is shown.
  EXPECT_FALSE(IsArcAppDialogViewAliveForTest());
  const auto& pending_requests3 =
      arc_usb_permission_manager->GetPendingRequestsForTest();
  EXPECT_EQ(0u, pending_requests3.size());

  // Checks permissions.
  EXPECT_TRUE(arc_usb_permission_manager->HasUsbAccessPermission(
      package0, guid, serial_number, vendor_id, product_id));
  EXPECT_TRUE(arc_usb_permission_manager->HasUsbAccessPermission(
      package1, guid, serial_number, vendor_id, product_id));
  EXPECT_FALSE(arc_usb_permission_manager->HasUsbAccessPermission(
      package2, guid, serial_number, vendor_id, product_id));
}

// Package is removed when permission request is queued.
IN_PROC_BROWSER_TEST_F(ArcAppDialogViewBrowserTest,
                       ArcUsbPermissionPackageUninstallFlow) {
  ArcUsbHostPermissionManager* arc_usb_permission_manager =
      ArcUsbHostPermissionManager::GetForBrowserContext(profile());
  DCHECK(arc_usb_permission_manager);

  InstallExtraPackage(1);
  InstallExtraPackage(2);

  const std::string package0 = base::StringPrintf("fake.package.%d", 0);
  const std::string package1 = base::StringPrintf("fake.package.%d", 1);
  const std::string package2 = base::StringPrintf("fake.package.%d", 2);
  const std::string guid = "TestGuidXXXXXX";
  const base::string16 device_name = base::UTF8ToUTF16("TestDeviceName");
  const base::string16 serial_number = base::UTF8ToUTF16("TestSerialNumber");
  uint16_t vendor_id = 123;
  uint16_t product_id = 456;

  // Package0 sends device access request.
  arc_usb_permission_manager->RequestUsbAccessPermission(
      package0, guid, device_name, serial_number, vendor_id, product_id,
      base::BindOnce(&ArcAppDialogViewBrowserTest::ConfirmCallback,
                     weak_ptr()));

  // Package1 sends device access request.
  arc_usb_permission_manager->RequestUsbAccessPermission(
      package1, guid, device_name, serial_number, vendor_id, product_id,
      base::BindOnce(&ArcAppDialogViewBrowserTest::ConfirmCallback,
                     weak_ptr()));

  // Package1 sends device access request.
  arc_usb_permission_manager->RequestUsbAccessPermission(
      package2, guid, device_name, serial_number, vendor_id, product_id,
      base::BindOnce(&ArcAppDialogViewBrowserTest::ConfirmCallback,
                     weak_ptr()));

  // Dialog is shown.
  EXPECT_TRUE(IsArcAppDialogViewAliveForTest());
  const auto& pending_requests0 =
      arc_usb_permission_manager->GetPendingRequestsForTest();
  EXPECT_EQ(3u, pending_requests0.size());
  EXPECT_EQ(package0, pending_requests0[0]->package_name);
  EXPECT_EQ(package1, pending_requests0[1]->package_name);
  EXPECT_EQ(package2, pending_requests0[2]->package_name);

  // Uninstall package0 and package2.
  UninstallPackage(package0);
  UninstallPackage(package2);
  const auto& pending_requests1 =
      arc_usb_permission_manager->GetPendingRequestsForTest();
  EXPECT_EQ(1u, pending_requests0.size());
  EXPECT_EQ(package1, pending_requests1[0]->package_name);

  // Accept the dialog. But callback is ignored as package0 is uninstalled.
  EXPECT_TRUE(CloseAppDialogViewAndConfirmForTest(true));
  content::RunAllPendingInMessageLoop();

  // Permision dialog for next request is shown.
  EXPECT_TRUE(IsArcAppDialogViewAliveForTest());
  const auto& pending_requests2 =
      arc_usb_permission_manager->GetPendingRequestsForTest();
  EXPECT_EQ(1u, pending_requests0.size());
  EXPECT_EQ(package1, pending_requests2[0]->package_name);

  EXPECT_TRUE(CloseAppDialogViewAndConfirmForTest(true));
  content::RunAllPendingInMessageLoop();

  EXPECT_FALSE(arc_usb_permission_manager->HasUsbAccessPermission(
      package0, guid, serial_number, vendor_id, product_id));
  EXPECT_TRUE(arc_usb_permission_manager->HasUsbAccessPermission(
      package1, guid, serial_number, vendor_id, product_id));
  EXPECT_FALSE(arc_usb_permission_manager->HasUsbAccessPermission(
      package2, guid, serial_number, vendor_id, product_id));
}

// Device is removed when permission request is queued.
IN_PROC_BROWSER_TEST_F(ArcAppDialogViewBrowserTest,
                       ArcUsbPermissionDeviceRemoveFlow) {
  ArcUsbHostPermissionManager* arc_usb_permission_manager =
      ArcUsbHostPermissionManager::GetForBrowserContext(profile());
  DCHECK(arc_usb_permission_manager);

  InstallExtraPackage(1);

  const std::string package0 = base::StringPrintf("fake.package.%d", 0);
  const std::string package1 = base::StringPrintf("fake.package.%d", 1);
  const std::string guid = "TestGuidXXXXXX";
  const base::string16 device_name = base::UTF8ToUTF16("TestDeviceName");
  const base::string16 serial_number = base::UTF8ToUTF16("TestSerialNumber");
  uint16_t vendor_id = 123;
  uint16_t product_id = 456;

  // Package0 sends device access request.
  arc_usb_permission_manager->RequestUsbAccessPermission(
      package0, guid, device_name, serial_number, vendor_id, product_id,
      base::BindOnce(&ArcAppDialogViewBrowserTest::ConfirmCallback,
                     weak_ptr()));

  // Package1 sends device access request.
  arc_usb_permission_manager->RequestUsbAccessPermission(
      package1, guid, device_name, serial_number, vendor_id, product_id,
      base::BindOnce(&ArcAppDialogViewBrowserTest::ConfirmCallback,
                     weak_ptr()));

  // Dialog is shown.
  EXPECT_TRUE(IsArcAppDialogViewAliveForTest());
  const auto& pending_requests0 =
      arc_usb_permission_manager->GetPendingRequestsForTest();
  EXPECT_EQ(2u, pending_requests0.size());
  EXPECT_EQ(package0, pending_requests0[0]->package_name);
  EXPECT_EQ(package1, pending_requests0[1]->package_name);

  // Device is removed.
  arc_usb_permission_manager->DeviceRemoved(guid);
  const auto& pending_requests1 =
      arc_usb_permission_manager->GetPendingRequestsForTest();
  EXPECT_EQ(0u, pending_requests1.size());

  // Accept the dialog. But callback is ignored as device is removed.
  EXPECT_TRUE(CloseAppDialogViewAndConfirmForTest(true));
  content::RunAllPendingInMessageLoop();

  EXPECT_FALSE(arc_usb_permission_manager->HasUsbAccessPermission(
      package0, guid, serial_number, vendor_id, product_id));
  EXPECT_FALSE(arc_usb_permission_manager->HasUsbAccessPermission(
      package1, guid, serial_number, vendor_id, product_id));
}

// User confirms/cancels ARC app uninstall. Note that the shortcut is removed
// when the app and the package are uninstalled since the shortcut and the app
// share same package.
IN_PROC_BROWSER_TEST_F(ArcAppDialogViewBrowserTest, UserConfirmsUninstall) {
  std::vector<std::string> app_ids = arc_app_list_pref()->GetAppIds();
  EXPECT_EQ(app_ids.size(), 2u);
  std::string package_name = base::StringPrintf("fake.package.%d", 0);
  std::string app_activity = base::StringPrintf("fake.app.%d.activity", 0);
  std::string app_id =
      arc_app_list_pref()->GetAppId(package_name, app_activity);

  AppListService* service = AppListService::Get();
  ASSERT_TRUE(service);
  service->ShowForProfile(browser()->profile());
  AppListControllerDelegate* controller(service->GetControllerDelegate());
  ASSERT_TRUE(controller);
  ShowArcAppUninstallDialog(browser()->profile(), controller, app_id);
  content::RunAllPendingInMessageLoop();

  EXPECT_TRUE(CloseAppDialogViewAndConfirmForTest(false));
  content::RunAllPendingInMessageLoop();
  app_ids = arc_app_list_pref()->GetAppIds();
  EXPECT_EQ(app_ids.size(), 2u);

  ShowArcAppUninstallDialog(browser()->profile(), controller, app_id);
  content::RunAllPendingInMessageLoop();

  EXPECT_TRUE(CloseAppDialogViewAndConfirmForTest(true));
  content::RunAllPendingInMessageLoop();
  app_ids = arc_app_list_pref()->GetAppIds();
  EXPECT_EQ(app_ids.size(), 0u);
  controller->DismissView();
}

// User confirms/cancels ARC app shortcut removal. Note that the app is not
// uninstalled when the shortcut is removed.
IN_PROC_BROWSER_TEST_F(ArcAppDialogViewBrowserTest,
                       UserConfirmsUninstallShortcut) {
  std::vector<std::string> app_ids = arc_app_list_pref()->GetAppIds();
  EXPECT_EQ(app_ids.size(), 2u);
  std::string package_name = base::StringPrintf("fake.package.%d", 0);
  std::string intent_uri = base::StringPrintf("Fake Shortcut uri %d", 0);
  std::string app_id = arc_app_list_pref()->GetAppId(package_name, intent_uri);

  AppListService* service = AppListService::Get();
  ASSERT_TRUE(service);
  service->ShowForProfile(browser()->profile());
  AppListControllerDelegate* controller(service->GetControllerDelegate());
  ASSERT_TRUE(controller);
  ShowArcAppUninstallDialog(browser()->profile(), controller, app_id);
  content::RunAllPendingInMessageLoop();

  EXPECT_TRUE(CloseAppDialogViewAndConfirmForTest(false));
  content::RunAllPendingInMessageLoop();
  app_ids = arc_app_list_pref()->GetAppIds();
  EXPECT_EQ(app_ids.size(), 2u);

  ShowArcAppUninstallDialog(browser()->profile(), controller, app_id);
  content::RunAllPendingInMessageLoop();

  EXPECT_TRUE(CloseAppDialogViewAndConfirmForTest(true));
  content::RunAllPendingInMessageLoop();
  app_ids = arc_app_list_pref()->GetAppIds();
  EXPECT_EQ(app_ids.size(), 1u);
  controller->DismissView();
}

}  // namespace arc
