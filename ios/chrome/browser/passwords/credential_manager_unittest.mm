// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/passwords/credential_manager.h"

#include "base/mac/foundation_util.h"
#include "base/test/scoped_task_environment.h"
#include "components/password_manager/core/browser/password_manager.h"
#include "components/password_manager/core/browser/stub_password_manager_client.h"
#include "components/password_manager/core/browser/test_password_store.h"
#include "components/password_manager/core/common/password_manager_pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "ios/chrome/browser/passwords/credential_manager_util.h"
#include "ios/chrome/browser/ssl/ios_security_state_tab_helper.h"
#include "ios/web/public/navigation_item.h"
#include "ios/web/public/navigation_manager.h"
#include "ios/web/public/ssl_status.h"
#include "ios/web/public/test/web_js_test.h"
#include "ios/web/public/test/web_test_with_web_state.h"
#include "net/ssl/ssl_connection_status_flags.h"
#include "net/test/cert_test_util.h"
#include "net/test/test_data_directory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

using password_manager::CredentialInfo;
using password_manager::GetCallback;
using password_manager::PreventSilentAccessCallback;
using password_manager::StoreCallback;
using password_manager::PasswordStore;
using password_manager::PasswordManager;
using password_manager::PasswordFormManager;
using password_manager::TestPasswordStore;
using testing::_;

namespace credential_manager {

namespace {

// Test hostname for cert verification.
constexpr char kHostName[] = "www.example.com";
// HTTPS origin corresponding to kHostName.
constexpr char kHttpsWebOrigin[] = "https://www.example.com/";
// HTTP origin corresponding to kHostName.
constexpr char kHttpWebOrigin[] = "http://www.example.com/";
// HTTP origin representing localhost. It should be considered secure.
constexpr char kLocalhostOrigin[] = "http://localhost";
// Origin with data scheme. It should be considered insecure.
constexpr char kDataUriSchemeOrigin[] = "data://www.example.com";
// File origin.
constexpr char kFileOrigin[] = "file://example_file";

// SSL certificate to load for testing.
constexpr char kCertFileName[] = "ok_cert.pem";

class MockPasswordManagerClient
    : public password_manager::StubPasswordManagerClient {
 public:
  MOCK_CONST_METHOD0(IsSavingAndFillingEnabledForCurrentPage, bool());
  MOCK_CONST_METHOD0(IsFillingEnabledForCurrentPage, bool());
  MOCK_METHOD0(OnCredentialManagerUsed, bool());
  MOCK_CONST_METHOD0(IsIncognito, bool());
  MOCK_METHOD0(NotifyStorePasswordCalled, void());
  MOCK_METHOD1(PromptUserToSavePasswordPtr, void(PasswordFormManager*));
  MOCK_METHOD3(PromptUserToChooseCredentialsPtr,
               bool(const std::vector<autofill::PasswordForm*>& local_forms,
                    const GURL& origin,
                    const CredentialsCallback& callback));

  MockPasswordManagerClient(PasswordStore* store)
      : store_(store), password_manager_(this) {
    prefs_.registry()->RegisterBooleanPref(
        password_manager::prefs::kCredentialsEnableAutosignin, true);
    prefs_.registry()->RegisterBooleanPref(
        password_manager::prefs::kWasAutoSignInFirstRunExperienceShown, true);
  }
  ~MockPasswordManagerClient() override {}

  bool PromptUserToSaveOrUpdatePassword(
      std::unique_ptr<PasswordFormManager> manager,
      bool update_password) override {
    manager_.swap(manager);
    PromptUserToSavePasswordPtr(manager_.get());
    return true;
  }
  PasswordStore* GetPasswordStore() const override { return store_; }
  PrefService* GetPrefs() override { return &prefs_; }
  const PasswordManager* GetPasswordManager() const override {
    return &password_manager_;
  }
  const GURL& GetLastCommittedEntryURL() const override {
    return last_committed_url_;
  }
  bool PromptUserToChooseCredentials(
      std::vector<std::unique_ptr<autofill::PasswordForm>> local_forms,
      const GURL& origin,
      const CredentialsCallback& callback) override {
    EXPECT_FALSE(local_forms.empty());
    const autofill::PasswordForm* form = local_forms[0].get();
    base::SequencedTaskRunnerHandle::Get()->PostTask(
        FROM_HERE,
        base::Bind(callback, base::Owned(new autofill::PasswordForm(*form))));
    std::vector<autofill::PasswordForm*> raw_forms(local_forms.size());
    std::transform(local_forms.begin(), local_forms.end(), raw_forms.begin(),
                   [](const std::unique_ptr<autofill::PasswordForm>& form) {
                     return form.get();
                   });
    PromptUserToChooseCredentialsPtr(raw_forms, origin, callback);
    return true;
  }

  PasswordFormManager* pending_manager() const { return manager_.get(); }

 private:
  TestingPrefServiceSimple prefs_;
  PasswordStore* store_;
  GURL last_committed_url_{kHttpsWebOrigin};
  PasswordManager password_manager_;
  std::unique_ptr<PasswordFormManager> manager_;

  DISALLOW_COPY_AND_ASSIGN(MockPasswordManagerClient);
};

}  // namespace

// TODO(crbug.com/435048): once JSCredentialManager is implemented and used
// from CredentialManager methods, mock it and write unit tests for
// CredentialManager methods SendGetResponse, SendStoreResponse and
// SendPreventSilentAccessResponse.
class CredentialManagerTest : public web::WebJsTest<web::WebTestWithWebState> {
 public:
  CredentialManagerTest()
      : web::WebJsTest<web::WebTestWithWebState>(@[ @"credential_manager" ]) {}
  void SetUp() override {
    WebTestWithWebState::SetUp();

    // Used indirectly by WebStateContentIsSecureHtml function.
    IOSSecurityStateTabHelper::CreateForWebState(web_state());

    store_ = new TestPasswordStore;
    store_->Init(syncer::SyncableService::StartSyncFlare(), nullptr);
    client_ = base::MakeUnique<MockPasswordManagerClient>(store_.get());
    manager_ = base::MakeUnique<CredentialManager>(client_.get(), web_state());

    ON_CALL(*client_, IsSavingAndFillingEnabledForCurrentPage())
        .WillByDefault(testing::Return(true));
    ON_CALL(*client_, IsFillingEnabledForCurrentPage())
        .WillByDefault(testing::Return(true));
    ON_CALL(*client_, OnCredentialManagerUsed())
        .WillByDefault(testing::Return(true));
    ON_CALL(*client_, IsIncognito()).WillByDefault(testing::Return(false));
  }

  void TearDown() override {
    manager_.reset();

    store_->Clear();
    store_->ShutdownOnUIThread();
    store_ = nullptr;

    WebTestWithWebState::TearDown();
  }

  // Updates SSLStatus on web_state()->GetNavigationManager()->GetVisibleItem()
  // with given |cert_status|, |security_style| and |content_status|.
  // SSLStatus fields |certificate|, |connection_status| and |cert_status_host|
  // are the same for all tests.
  void UpdateSslStatus(net::CertStatus cert_status,
                       web::SecurityStyle security_style,
                       web::SSLStatus::ContentStatusFlags content_status) {
    scoped_refptr<net::X509Certificate> cert =
        net::ImportCertFromFile(net::GetTestCertsDirectory(), kCertFileName);
    web::SSLStatus& ssl =
        web_state()->GetNavigationManager()->GetVisibleItem()->GetSSL();
    ssl.security_style = security_style;
    ssl.certificate = cert;
    ssl.cert_status = cert_status;
    ssl.connection_status = net::SSL_CONNECTION_VERSION_SSL3;
    ssl.content_status = content_status;
    ssl.cert_status_host = kHostName;
  }

 protected:
  // base::test::ScopedTaskEnvironment scoped_task_environment_;
  std::unique_ptr<MockPasswordManagerClient> client_;
  std::unique_ptr<CredentialManager> manager_;
  scoped_refptr<TestPasswordStore> store_;
};

// Tests storing and then retrieving a PasswordCredential.
TEST_F(CredentialManagerTest, StoreAndGetPasswordCredential) {
  // Inject JavaScript and set up secure context.
  LoadHtml(@"<html></html>", GURL(kHttpsWebOrigin));
  LoadHtmlAndInject(@"<html></html>");
  UpdateSslStatus(net::CERT_STATUS_IS_EV, web::SECURITY_STYLE_AUTHENTICATED,
                  web::SSLStatus::NORMAL_CONTENT);

  // Call API method |store|.
  ExecuteJavaScript(
      @"var credential = new PasswordCredential({"
       "id: 'id',"
       "password: 'pencil',"
       "name: 'name',"
       "iconURL: 'https://example.com/icon.png'"
       "});"
       "navigator.credentials.store(credential).then(function() { "
       "test_credential_stored_ = true; });");

  // Wait for credential to be stored.
  WaitForBackgroundTasks();
  client_->pending_manager()->Save();
  WaitForCondition(^{
    return !store_->IsEmpty();
  });

  // Wait for the Promise to be resolved.
  WaitForCondition(^{
    return [@YES isEqual:ExecuteJavaScript(@"test_credential_stored_")];
  });

  // Call API method |get|.
  ExecuteJavaScript(
      @"navigator.credentials.get({password: true, mediation: "
      @"'silent'}).then(function(credential) { test_credential_ = credential; "
      @"test_promise_resolved_ = true; })");

  // Wait for PasswordCredential to be obtained and for Promise to be resolved.
  WaitForCondition(^{
    return [@YES isEqual:ExecuteJavaScript(@"test_promise_resolved_")];
  });

  // Check PasswordCredential fields.
  ASSERT_NSEQ(@"password", ExecuteJavaScript(@"test_credential_.type"));
  ASSERT_NSEQ(@"id", ExecuteJavaScript(@"test_credential_.id"));
  ASSERT_NSEQ(@"name", ExecuteJavaScript(@"test_credential_.name"));
  ASSERT_NSEQ(@"pencil", ExecuteJavaScript(@"test_credential_.password"));
  ASSERT_NSEQ(@"https://example.com/icon.png",
              ExecuteJavaScript(@"test_credential_.iconURL"));
}

// Tests storing and then retrieving a FederatedCredential.
TEST_F(CredentialManagerTest, StoreAndGetFederatedCredential) {
  // Inject JavaScript and set up secure context.
  LoadHtml(@"<html></html>", GURL(kHttpsWebOrigin));
  LoadHtmlAndInject(@"<html></html>");
  UpdateSslStatus(net::CERT_STATUS_IS_EV, web::SECURITY_STYLE_AUTHENTICATED,
                  web::SSLStatus::NORMAL_CONTENT);

  // Call API method |store|.
  ExecuteJavaScript(
      @"var credential = new FederatedCredential({"
       "id: 'id',"
       "provider: 'https://example.com',"
       "name: 'name',"
       "iconURL: 'https://example.com/icon.png'"
       "});"
       "navigator.credentials.store(credential).then(function() { "
       "test_credential_stored_ = true; });");

  // Wait for credential to be stored.
  WaitForBackgroundTasks();
  client_->pending_manager()->Save();
  WaitForCondition(^{
    return !store_->IsEmpty();
  });

  // Wait for the Promise to be resolved.
  WaitForCondition(^{
    return [@YES isEqual:ExecuteJavaScript(@"test_credential_stored_")];
  });

  // Call API method |get|.
  ExecuteJavaScript(
      @"navigator.credentials.get({providers: ['https://example.com'], "
      @"mediation: 'silent'}).then(function(credential) { test_credential_ = "
      @"credential; test_promise_resolved_ = true; })");

  // Wait for FederatedCredential to be obtained and for Promise to be resolved.
  WaitForCondition(^{
    return [@YES isEqual:ExecuteJavaScript(@"test_promise_resolved_")];
  });

  // Check FederatedCredential fields.
  ASSERT_NSEQ(@"federated", ExecuteJavaScript(@"test_credential_.type"));
  ASSERT_NSEQ(@"id", ExecuteJavaScript(@"test_credential_.id"));
  ASSERT_NSEQ(@"name", ExecuteJavaScript(@"test_credential_.name"));
  ASSERT_NSEQ(@"https://example.com",
              ExecuteJavaScript(@"test_credential_.provider"));
  ASSERT_NSEQ(@"https://example.com/icon.png",
              ExecuteJavaScript(@"test_credential_.iconURL"));
}

// Tests that storing a credential from insecure context will fail.
TEST_F(CredentialManagerTest, StoreCredentialFromInsecureContext) {
  // Inject JavaScript, set up WebState to have mixed content.
  LoadHtml(@"<html></html>", GURL(kHttpsWebOrigin));
  LoadHtmlAndInject(@"<html></html>");
  UpdateSslStatus(net::CERT_STATUS_IS_EV, web::SECURITY_STYLE_AUTHENTICATED,
                  web::SSLStatus::DISPLAYED_INSECURE_CONTENT);

  // Call API method |store|.
  ExecuteJavaScript(
      @"var credential = new PasswordCredential({"
       "id: 'id',"
       "password: 'pencil',"
       "name: 'name',"
       "iconURL: 'https://example.com/icon.png'"
       "});"
       "navigator.credentials.store(credential).then(function() { "
       "test_credential_stored_ = true; });");

  WaitForBackgroundTasks();

  // Expect that user will NOT be prompted to save or update password.
  EXPECT_CALL(*client_, PromptUserToSavePasswordPtr(_)).Times(0);

  // Expect that PasswordManagerClient methods used by
  // CredentialManagerImpl::Store will not be called.
  EXPECT_CALL(*client_, OnCredentialManagerUsed()).Times(0);
  EXPECT_CALL(*client_, IsSavingAndFillingEnabledForCurrentPage()).Times(0);
}

class WebStateContentIsSecureHtmlTest : public CredentialManagerTest {};

// Tests that HTTPS websites with valid SSL certificate are recognized as
// secure.
TEST_F(WebStateContentIsSecureHtmlTest, AcceptHttpsUrls) {
  LoadHtml(@"<html></html>", GURL(kHttpsWebOrigin));
  UpdateSslStatus(net::CERT_STATUS_IS_EV, web::SECURITY_STYLE_AUTHENTICATED,
                  web::SSLStatus::NORMAL_CONTENT);
  EXPECT_TRUE(WebStateContentIsSecureHtml(web_state()));
}

// Tests that WebStateContentIsSecureHtml returns false for HTTP origin.
TEST_F(WebStateContentIsSecureHtmlTest, HttpIsNotSecureContext) {
  LoadHtml(@"<html></html>", GURL(kHttpWebOrigin));
  EXPECT_FALSE(WebStateContentIsSecureHtml(web_state()));
}

// Tests that WebStateContentIsSecureHtml returns false for HTTPS origin with
// valid SSL certificate but mixed contents.
TEST_F(WebStateContentIsSecureHtmlTest, InsecureContent) {
  LoadHtml(@"<html></html>", GURL(kHttpsWebOrigin));
  UpdateSslStatus(net::CERT_STATUS_IS_EV, web::SECURITY_STYLE_AUTHENTICATED,
                  web::SSLStatus::DISPLAYED_INSECURE_CONTENT);
  EXPECT_FALSE(WebStateContentIsSecureHtml(web_state()));
}

// Tests that WebStateContentIsSecureHtml returns false for HTTPS origin with
// invalid SSL certificate.
TEST_F(WebStateContentIsSecureHtmlTest, InvalidSslCertificate) {
  LoadHtml(@"<html></html>", GURL(kHttpsWebOrigin));
  UpdateSslStatus(net::CERT_STATUS_INVALID, web::SECURITY_STYLE_UNAUTHENTICATED,
                  web::SSLStatus::NORMAL_CONTENT);
  EXPECT_FALSE(WebStateContentIsSecureHtml(web_state()));
}

// Tests that data:// URI scheme is not accepted as secure context.
TEST_F(WebStateContentIsSecureHtmlTest, DataUriSchemeIsNotSecureContext) {
  LoadHtml(@"<html></html>", GURL(kDataUriSchemeOrigin));
  EXPECT_FALSE(WebStateContentIsSecureHtml(web_state()));
}

// Tests that localhost is accepted as secure context.
TEST_F(WebStateContentIsSecureHtmlTest, LocalhostIsSecureContext) {
  LoadHtml(@"<html></html>", GURL(kLocalhostOrigin));
  EXPECT_TRUE(WebStateContentIsSecureHtml(web_state()));
}

// Tests that file origin is accepted as secure context.
TEST_F(WebStateContentIsSecureHtmlTest, FileIsSecureContext) {
  LoadHtml(@"<html></html>", GURL(kFileOrigin));
  EXPECT_TRUE(WebStateContentIsSecureHtml(web_state()));
}

// Tests that content must be HTML.
TEST_F(WebStateContentIsSecureHtmlTest, ContentMustBeHtml) {
  // No HTML is loaded on purpose, so that web_state()->ContentIsHTML() will
  // return false.
  EXPECT_FALSE(WebStateContentIsSecureHtml(web_state()));
}

}  // namespace credential_manager
