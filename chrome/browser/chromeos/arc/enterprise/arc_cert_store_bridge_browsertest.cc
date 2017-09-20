// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <vector>

#include "base/base64.h"
#include "base/command_line.h"
#include "base/json/json_writer.h"
#include "base/run_loop.h"
#include "chrome/browser/chromeos/arc/arc_service_launcher.h"
#include "chrome/browser/chromeos/arc/enterprise/arc_cert_store_bridge.h"
#include "chrome/browser/chromeos/platform_keys/key_permissions.h"
#include "chrome/browser/chromeos/platform_keys/platform_keys.h"
#include "chrome/browser/chromeos/policy/user_policy_test_helper.h"
#include "chrome/browser/chromeos/profiles/profile_helper.h"
#include "chrome/browser/net/nss_context.h"
#include "chrome/browser/policy/profile_policy_connector.h"
#include "chrome/browser/policy/profile_policy_connector_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chromeos/chromeos_switches.h"
#include "components/arc/arc_bridge_service.h"
#include "components/arc/arc_service_manager.h"
#include "components/arc/arc_util.h"
#include "components/arc/common/cert_store.mojom.h"
#include "components/policy/policy_constants.h"
#include "content/public/browser/browser_thread.h"
#include "crypto/scoped_test_system_nss_key_slot.h"
#include "extensions/browser/extension_system.h"
#include "net/cert/nss_cert_database.h"
#include "net/test/cert_test_util.h"
#include "net/test/test_data_directory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

constexpr char kFakeUserName[] = "test@example.com";
constexpr char kFakePackageName[] = "fake.package.name";
constexpr char kFakeExtensionId[] = "fakeextensionid";

}  // namespace

namespace arc {

class FakeArcCertStoreInstance : public mojom::CertStoreInstance {
 public:
  // mojom::CertStoreInstance:
  void Init(mojom::CertStoreHostPtr host) override { host_ = std::move(host); }

  void OnKeyPermissionsChanged(
      const std::vector<std::string>& permissions) override {
    permissions_ = permissions;
  }

  const std::vector<std::string>& permissions() const { return permissions_; }

 private:
  mojom::CertStoreHostPtr host_;
  std::vector<std::string> permissions_;
};

class ArcCertStoreBridgeTest : public InProcessBrowserTest {
 protected:
  ArcCertStoreBridgeTest() {}

  // InProcessBrowserTest:
  ~ArcCertStoreBridgeTest() override = default;

  void SetUpCommandLine(base::CommandLine* command_line) override {
    InProcessBrowserTest::SetUpCommandLine(command_line);

    arc::SetArcAvailableCommandLineForTesting(command_line);

    policy_helper_ =
        std::make_unique<policy::UserPolicyTestHelper>(kFakeUserName);
    policy_helper_->Init(
        base::DictionaryValue() /* empty mandatory policy */,
        base::DictionaryValue() /* empty recommended policy */);
    policy_helper_->UpdateCommandLine(command_line);

    command_line->AppendSwitchASCII(chromeos::switches::kLoginUser,
                                    kFakeUserName);
    command_line->AppendSwitchASCII(chromeos::switches::kLoginProfile, "user");
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    policy_helper_->WaitForInitialPolicy(browser()->profile());

    {
      base::RunLoop loop;
      content::BrowserThread::PostTaskAndReply(
          content::BrowserThread::IO, FROM_HERE,
          base::BindOnce(&ArcCertStoreBridgeTest::SetUpTestSystemSlotOnIO,
                         base::Unretained(this),
                         browser()->profile()->GetResourceContext()),
          loop.QuitClosure());
      loop.Run();
    }

    // Init ArcSessionManager for testing.
    ArcServiceLauncher::Get()->ResetForTesting();

    chromeos::ProfileHelper::SetAlwaysReturnPrimaryUserForTesting(true);

    browser()->profile()->GetPrefs()->SetBoolean(prefs::kArcSignedIn, true);
    browser()->profile()->GetPrefs()->SetBoolean(prefs::kArcTermsAccepted,
                                                 true);

    ArcServiceLauncher::Get()->OnPrimaryUserProfilePrepared(
        browser()->profile());

    instance_ = std::make_unique<FakeArcCertStoreInstance>();
    ASSERT_TRUE(arc_bridge());
    arc_bridge()->cert_store()->SetInstance(instance_.get());

    {
      base::RunLoop loop;
      GetNSSCertDatabaseForProfile(
          browser()->profile(),
          base::Bind(&ArcCertStoreBridgeTest::SetUpTestClientCerts,
                     base::Unretained(this), loop.QuitClosure()));
      loop.Run();
    }
  }

  void TearDownOnMainThread() override {
    ASSERT_TRUE(arc_bridge());
    arc_bridge()->cert_store()->SetInstance(nullptr);

    // Since ArcServiceLauncher is (re-)set up with profile() in
    // SetUpOnMainThread() it is necessary to Shutdown() before the profile()
    // is destroyed. ArcServiceLauncher::Shutdown() will be called again on
    // fixture destruction (because it is initialized with the original Profile
    // instance in fixture, once), but it should be no op.
    ArcServiceLauncher::Get()->Shutdown();
    chromeos::ProfileHelper::SetAlwaysReturnPrimaryUserForTesting(false);

    base::RunLoop loop;
    content::BrowserThread::PostTaskAndReply(
        content::BrowserThread::IO, FROM_HERE,
        base::BindOnce(&ArcCertStoreBridgeTest::TearDownTestSystemSlotOnIO,
                       base::Unretained(this)),
        loop.QuitClosure());
    loop.Run();
  }

  void SetUpTestSystemSlotOnIO(content::ResourceContext* context) {
    DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
    test_system_slot_ = std::make_unique<crypto::ScopedTestSystemNSSKeySlot>();
    ASSERT_TRUE(test_system_slot_->ConstructedSuccessfully());
  }

  void TearDownTestSystemSlotOnIO() {
    DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
    test_system_slot_.reset();
  }

  ArcBridgeService* arc_bridge() {
    return ArcServiceManager::Get()->arc_bridge_service();
  }

  FakeArcCertStoreInstance* instance() { return instance_.get(); }

  void SetCorporateKeyUsagePolicy(const std::string& package_name) {
    // Set up the test policy that gives app the permission to access
    // corporate keys.
    base::DictionaryValue key_permissions_policy;
    {
      std::unique_ptr<base::DictionaryValue> cert_key_permission(
          new base::DictionaryValue);
      cert_key_permission->SetKey("allowCorporateKeyUsage", base::Value(true));
      key_permissions_policy.SetWithoutPathExpansion(
          package_name, std::move(cert_key_permission));
    }

    std::string key_permissions_policy_str;
    base::JSONWriter::WriteWithOptions(key_permissions_policy,
                                       base::JSONWriter::OPTIONS_PRETTY_PRINT,
                                       &key_permissions_policy_str);

    base::DictionaryValue user_policy;
    user_policy.SetKey(policy::key::kKeyPermissions,
                       base::Value(key_permissions_policy_str));

    policy_helper_->UpdatePolicy(
        user_policy, base::DictionaryValue() /* empty recommended policy */,
        browser()->profile());
  }

  void RegisterCorporateKeys() {
    policy::ProfilePolicyConnector* const policy_connector =
        policy::ProfilePolicyConnectorFactory::GetForBrowserContext(
            browser()->profile());

    extensions::StateStore* const state_store =
        extensions::ExtensionSystem::Get(browser()->profile())->state_store();

    chromeos::KeyPermissions permissions(
        policy_connector->IsManaged(), browser()->profile()->GetPrefs(),
        policy_connector->policy_service(), state_store);

    base::RunLoop run_loop;
    permissions.GetPermissionsForExtension(
        kFakeExtensionId,
        base::Bind(&ArcCertStoreBridgeTest::GotPermissionsForExtension,
                   base::Unretained(this), run_loop.QuitClosure()));
    run_loop.Run();
  }

  // Register only client_cert1_ for corporate usage to test that
  // client_cert2_ is not allowed.
  void GotPermissionsForExtension(
      const base::Closure& done_callback,
      std::unique_ptr<chromeos::KeyPermissions::PermissionsForExtension>
          permissions_for_ext) {
    std::string client_cert1_spki(
        client_cert1_->derPublicKey.data,
        client_cert1_->derPublicKey.data + client_cert1_->derPublicKey.len);
    permissions_for_ext->RegisterKeyForCorporateUsage(client_cert1_spki);

    done_callback.Run();
  }

  void SetUpTestClientCerts(const base::Closure& done_callback,
                            net::NSSCertDatabase* cert_db) {
    net::ImportClientCertAndKeyFromFile(
        net::GetTestCertsDirectory(), "client_1.pem", "client_1.pk8",
        cert_db->GetPrivateSlot().get(), &client_cert1_);
    net::ImportClientCertAndKeyFromFile(
        net::GetTestCertsDirectory(), "client_2.pem", "client_2.pk8",
        cert_db->GetPrivateSlot().get(), &client_cert2_);
    done_callback.Run();
  }

  void OnCertificatesListed(
      const std::vector<mojom::CertificatePtr>& expected_certs,
      std::vector<mojom::CertificatePtr> certs) {
    ASSERT_EQ(certs.size(), expected_certs.size());
    for (const auto& cert : certs) {
      bool found = false;
      for (const auto& expected_cert : expected_certs) {
        if (expected_cert->alias == cert->alias &&
            expected_cert->cert == cert->cert) {
          found = true;
          break;
        }
      }
      ASSERT_TRUE(found);
    }
  }

  void ListCertificatesTest(ArcCertStoreBridge* cert_store_bridge,
                            std::vector<mojom::CertificatePtr> expected_certs) {
    cert_store_bridge->ListCertificates(
        base::Bind(&ArcCertStoreBridgeTest::OnCertificatesListed,
                   base::Unretained(this), std::move(expected_certs)));
  }

  net::ScopedCERTCertificate client_cert1_;
  net::ScopedCERTCertificate client_cert2_;

 private:
  std::unique_ptr<policy::UserPolicyTestHelper> policy_helper_;
  std::unique_ptr<FakeArcCertStoreInstance> instance_;
  std::unique_ptr<crypto::ScopedTestSystemNSSKeySlot> test_system_slot_;

  DISALLOW_COPY_AND_ASSIGN(ArcCertStoreBridgeTest);
};

// Test OnKeyPermissionsChanged().
IN_PROC_BROWSER_TEST_F(ArcCertStoreBridgeTest, KeyPermissionsTest) {
  ArcCertStoreBridge* cert_store_bridge =
      ArcCertStoreBridge::GetForBrowserContext(browser()->profile());
  ASSERT_TRUE(cert_store_bridge);
  // Corporate usage keys are not allowed to any app/extension:
  ASSERT_EQ(0U, instance()->permissions().size());

  // Allow corporate usage keys to ARC app.
  SetCorporateKeyUsagePolicy(kFakePackageName);

  ASSERT_EQ(1U, instance()->permissions().size());
  ASSERT_EQ(kFakePackageName, instance()->permissions()[0]);

  // Allow corporate usage keys only to Chrome extensions.
  SetCorporateKeyUsagePolicy(kFakeExtensionId);

  ASSERT_EQ(0U, instance()->permissions().size());
}

// Test ListCertificates() with no corporate usage keys.
IN_PROC_BROWSER_TEST_F(ArcCertStoreBridgeTest, ListCertificatesBasicTest) {
  ArcCertStoreBridge* cert_store_bridge =
      ArcCertStoreBridge::GetForBrowserContext(browser()->profile());
  ASSERT_TRUE(cert_store_bridge);

  // Allow corporate usage keys to ARC app.
  SetCorporateKeyUsagePolicy(kFakePackageName);

  ListCertificatesTest(cert_store_bridge, std::vector<mojom::CertificatePtr>());
}

// Test ListCertificates() with 2 corporate usage keys.
IN_PROC_BROWSER_TEST_F(ArcCertStoreBridgeTest, ListCertificatesTest) {
  ArcCertStoreBridge* cert_store_bridge =
      ArcCertStoreBridge::GetForBrowserContext(browser()->profile());
  ASSERT_TRUE(cert_store_bridge);

  // Allow corporate usage keys to ARC app.
  SetCorporateKeyUsagePolicy(kFakePackageName);

  // Register corporate certificates.
  RegisterCorporateKeys();

  auto mojom_cert1 = mojom::Certificate::New();
  mojom_cert1->alias = client_cert1_->nickname;
  base::Base64Encode(base::StringPiece(reinterpret_cast<char const*>(
                                           client_cert1_->derCert.data),
                                       client_cert1_->derCert.len),
                     &mojom_cert1->cert);

  std::vector<mojom::CertificatePtr> expected_certs;
  expected_certs.emplace_back(std::move(mojom_cert1));

  ListCertificatesTest(cert_store_bridge, std::move(expected_certs));
}

}  // namespace arc
