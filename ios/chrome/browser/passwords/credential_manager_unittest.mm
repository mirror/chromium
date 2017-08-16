#import "ios/chrome/browser/passwords/credential_manager.h"

#include "base/strings/string16.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/scoped_task_environment.h"
#include "components/password_manager/core/browser/password_manager.h"
#include "components/password_manager/core/browser/test_password_store.h"
#include "components/password_manager/core/browser/stub_password_manager_client.h"
#include "components/password_manager/core/common/password_manager_pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace password_manager {

namespace {

const char kTestWebOrigin[] = "https://example.com/";

class MockPasswordManagerClient : public StubPasswordManagerClient {
 public:
  MOCK_CONST_METHOD0(IsSavingAndFillingEnabledForCurrentPage, bool());
  MOCK_CONST_METHOD0(IsFillingEnabledForCurrentPage, bool());
  MOCK_METHOD0(OnCredentialManagerUsed, bool());
  MOCK_CONST_METHOD0(IsIncognito, bool());
  MOCK_METHOD0(NotifyUserAutoSigninPtr, bool());
  MOCK_METHOD1(NotifyUserCouldBeAutoSignedInPtr,
               bool(autofill::PasswordForm* form));
  MOCK_METHOD0(NotifyStorePasswordCalled, void());
  MOCK_METHOD1(PromptUserToSavePasswordPtr, void(PasswordFormManager*));
  MOCK_METHOD3(PromptUserToChooseCredentialsPtr,
               bool(const std::vector<autofill::PasswordForm*>& local_forms,
                    const GURL& origin,
                    const CredentialsCallback& callback));

  explicit MockPasswordManagerClient(PasswordStore* store)
      : store_(store), password_manager_(this) {
    prefs_.registry()->RegisterBooleanPref(prefs::kCredentialsEnableAutosignin,
                                           true);
    prefs_.registry()->RegisterBooleanPref(
        prefs::kWasAutoSignInFirstRunExperienceShown, true);
  }
  ~MockPasswordManagerClient() override {}

  bool PromptUserToSaveOrUpdatePassword(
      std::unique_ptr<PasswordFormManager> manager,
      bool update_password) override {
    manager_.swap(manager);
    PromptUserToSavePasswordPtr(manager_.get());
    return true;
  }

  void NotifyUserCouldBeAutoSignedIn(
      std::unique_ptr<autofill::PasswordForm> form) override {
    NotifyUserCouldBeAutoSignedInPtr(form.get());
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

  void NotifyUserAutoSignin(
      std::vector<std::unique_ptr<autofill::PasswordForm>> local_forms,
      const GURL& origin) override {
    EXPECT_FALSE(local_forms.empty());
    NotifyUserAutoSigninPtr();
  }

  PasswordFormManager* pending_manager() const { return manager_.get(); }

 private:
  TestingPrefServiceSimple prefs_;
  PasswordStore* store_;
  std::unique_ptr<PasswordFormManager> manager_;
  PasswordManager password_manager_;
  GURL last_committed_url_{kTestWebOrigin};

  DISALLOW_COPY_AND_ASSIGN(MockPasswordManagerClient);
};

}  // namespace

class CredentialManagerTest : public testing::Test {
 public:
  void SetUp() override {
    store_ = new TestPasswordStore;
    store_->Init(syncer::SyncableService::StartSyncFlare(), nullptr);
    client_ = base::MakeUnique<MockPasswordManagerClient>(store_.get());
    credential_manager_ = base::MakeUnique<CredentialManager>(client_.get(), nullptr);

    ON_CALL(*client_, IsSavingAndFillingEnabledForCurrentPage())
        .WillByDefault(testing::Return(true));
    ON_CALL(*client_, IsFillingEnabledForCurrentPage())
        .WillByDefault(testing::Return(true));
    ON_CALL(*client_, OnCredentialManagerUsed())
        .WillByDefault(testing::Return(true));
    ON_CALL(*client_, IsIncognito()).WillByDefault(testing::Return(false));
  }

  void TearDown() override {
    credential_manager_.reset();
    client_.reset();

    store_->ShutdownOnUIThread();

    // Clean up the password store asynchronously.
    RunAllPendingTasks();
  }

  void RunAllPendingTasks() { scoped_task_environment_.RunUntilIdle(); }

 protected:
  base::test::ScopedTaskEnvironment scoped_task_environment_;
  scoped_refptr<TestPasswordStore> store_;
  std::unique_ptr<MockPasswordManagerClient> client_;
  std::unique_ptr<CredentialManager> credential_manager_;
};

// Checks that HandleScriptCommand should return false
// for message with wrong or no |requestId| field.
TEST_F(CredentialManagerTest, MessageWithInvalidRequestId) {
  base::DictionaryValue JSON;

  // No |requestId|
  JSON.SetString("command", "navigator.preventSilentAccess");
  EXPECT_FALSE(credential_manager_->HandleScriptCommand(JSON, GURL(kTestWebOrigin), false));

  // |requestId| is not an integer value
  JSON.SetString("requestId", "eight");
  EXPECT_FALSE(credential_manager_->HandleScriptCommand(JSON, GURL(kTestWebOrigin), false));
}

// Checks that HandleScriptCommand should return false
// for message with wrong or no |command| field.
TEST_F(CredentialManagerTest, MessageWithInvalidCommand) {
  base::DictionaryValue JSON;

  // No |command|
  JSON.SetInteger("requestId", 1);
  JSON.SetString("id", "john@doe.com");
  JSON.SetString("type", "PasswordCredential");
  JSON.SetString("name", "John Doe");
  JSON.SetString("iconURL", "https://www.google.com/favicon.ico");
  JSON.SetString("password", "admin123");
  EXPECT_FALSE(credential_manager_->HandleScriptCommand(JSON, GURL(kTestWebOrigin), false));

  // Invalid |command|
  // Should be credentials.store
  JSON.SetString("command", "store");
  EXPECT_FALSE(credential_manager_->HandleScriptCommand(JSON, GURL(kTestWebOrigin), false));

  // Invalid |command|
  // credentials.create should be implemented entirely at JS side.
  JSON.SetString("command", "credentials.create");
  EXPECT_FALSE(credential_manager_->HandleScriptCommand(JSON, GURL(kTestWebOrigin), false));
}

// Checks that HandleScriptCommand should return false for
// message with wrong or no |type| field.
TEST_F(CredentialManagerTest, InvalidCredentialType) {
  base::DictionaryValue JSON;

  // Construct a PasswordCredential with no |type| value
  JSON.SetInteger("requestId", 1);
  JSON.SetString("command", "credentials.store");
  JSON.SetString("id", "john@doe.com");
  JSON.SetString("name", "John Doe");
  JSON.SetString("iconURL", "https://www.google.com/favicon.ico");
  JSON.SetString("password", "admin123");

  EXPECT_FALSE(credential_manager_->HandleScriptCommand(JSON, GURL(kTestWebOrigin), false));

  // Set the wrong |type| value.
  JSON.SetString("type", "Credential");
  EXPECT_FALSE(credential_manager_->HandleScriptCommand(JSON, GURL(kTestWebOrigin), false));
}

// Checks that passing valid JSON describing PasswordCredential
// results in storing the credential in password store.
TEST_F(CredentialManagerTest, StorePasswordCredential) {
  base::DictionaryValue JSON;

  JSON.SetInteger("requestId", 1);
  JSON.SetString("command", "credentials.store");
  JSON.SetString("id", "john@doe.com");
  JSON.SetString("name", "John Doe");
  JSON.SetString("iconURL", "https://www.google.com/favicon.ico");
  JSON.SetString("password", "admin123");
  JSON.SetString("type", "PasswordCredential");

  EXPECT_TRUE(credential_manager_->HandleScriptCommand(JSON, GURL(kTestWebOrigin), false));
  // Allow the PasswordFormManager to talk to the password store.
  RunAllPendingTasks();

  client_->pending_manager()->Save();
  // Let PasswordFormManager::Save pending task finish.
  RunAllPendingTasks();

  EXPECT_FALSE(store_->IsEmpty());
  TestPasswordStore::PasswordMap passwords = store_->stored_passwords();
  EXPECT_EQ(passwords[kTestWebOrigin].size(), 1u);
  autofill::PasswordForm form = *passwords[kTestWebOrigin].begin();

  // Check if stored PasswordForm has all the same values as expected.
  EXPECT_EQ(form.icon_url, GURL("https://www.google.com/favicon.ico"));
  EXPECT_EQ(form.display_name, base::ASCIIToUTF16("John Doe"));
  EXPECT_EQ(form.password_value, base::ASCIIToUTF16("admin123"));
  EXPECT_EQ(form.username_value, base::ASCIIToUTF16("john@doe.com"));
  EXPECT_EQ(form.type, autofill::PasswordForm::Type::TYPE_API);
}

// Checks that passing valid JSON describing FederatedCredential
// results in storing the credential in password store.
TEST_F(CredentialManagerTest, StoreFederatedCredential) {
  base::DictionaryValue JSON;

  JSON.SetInteger("requestId", 1);
  JSON.SetString("command", "credentials.store");
  JSON.SetString("id", "john@doe.com");
  JSON.SetString("name", "John Doe");
  JSON.SetString("iconURL", "https://www.google.com/favicon.ico");
  JSON.SetString("provider", kTestWebOrigin);
  JSON.SetString("type", "FederatedCredential");

  EXPECT_TRUE(credential_manager_->HandleScriptCommand(JSON, GURL(kTestWebOrigin), false));
  // Allow the PasswordFormManager to talk to the password store.
  RunAllPendingTasks();

  client_->pending_manager()->Save();
  // Let PasswordFormManager::Save pending task finish.
  RunAllPendingTasks();

  EXPECT_FALSE(store_->IsEmpty());
  TestPasswordStore::PasswordMap passwords = store_->stored_passwords();
  std::string signon_realm = "federation://example.com/example.com";
  EXPECT_EQ(passwords[signon_realm].size(), 1u);
  autofill::PasswordForm form = *passwords[signon_realm].begin();

  // Check if stored PasswordForm has all the same values as expected.
  EXPECT_EQ(form.icon_url, GURL("https://www.google.com/favicon.ico"));
  EXPECT_EQ(form.display_name, base::ASCIIToUTF16("John Doe"));
  EXPECT_EQ(form.federation_origin.scheme(), std::string("https"));
  EXPECT_EQ(form.federation_origin.host(), std::string("example.com"));
  EXPECT_EQ(form.username_value, base::ASCIIToUTF16("john@doe.com"));
  EXPECT_EQ(form.type, autofill::PasswordForm::Type::TYPE_API);
}

// Checks if CredentialRequestOptions are parsed correctly.
TEST_F(CredentialManagerTest, ParseGetArguments) {
  base::DictionaryValue JSON;

  // Check if default value of |include_passwords| is false.
  EXPECT_FALSE(CredentialManager::ParseIncludePasswords(JSON));
  // Check if value of |include_passwords| is parsed correctly.
  JSON.SetBoolean("password", false);
  EXPECT_FALSE(CredentialManager::ParseIncludePasswords(JSON));
  JSON.SetBoolean("password", true);
  EXPECT_TRUE(CredentialManager::ParseIncludePasswords(JSON));

  // Check if default value of |mediation| is kOptional.
  EXPECT_EQ(CredentialMediationRequirement::kOptional, CredentialManager::ParseMediationRequirement(JSON));
  // Check if value of |mediation| is parsed correctly.
  JSON.SetString("mediation", "silent");
  EXPECT_EQ(CredentialMediationRequirement::kSilent, CredentialManager::ParseMediationRequirement(JSON));
  JSON.SetString("mediation", "required");
  EXPECT_EQ(CredentialMediationRequirement::kRequired, CredentialManager::ParseMediationRequirement(JSON));
  JSON.SetString("mediation", "optional");
  EXPECT_EQ(CredentialMediationRequirement::kOptional, CredentialManager::ParseMediationRequirement(JSON));

  // Check if value of |federations| if parsed correctly.
  JSON.Clear();
  std::unique_ptr<base::ListValue> list_ptr = base::MakeUnique<base::ListValue>();
  list_ptr->AppendString(kTestWebOrigin);
  list_ptr->AppendString("https://google.com");
  JSON.SetList("providers", std::move(list_ptr));
  std::vector<GURL> federations = CredentialManager::ParseFederations(JSON);
  EXPECT_THAT(federations, testing::ElementsAre(GURL(kTestWebOrigin), GURL("https://google.com")));
}

}  // namespace password_manager
