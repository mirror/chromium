// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/sync/sync_setup_wizard.h"

#include "base/json/json_writer.h"
#include "base/memory/scoped_ptr.h"
#include "base/stl_util-inl.h"
#include "base/utf_string_conversions.h"
#include "chrome/browser/prefs/pref_service.h"
#include "chrome/browser/sync/engine/syncapi.h"
#include "chrome/browser/sync/profile_sync_factory_mock.h"
#include "chrome/browser/sync/profile_sync_service.h"
#include "chrome/browser/sync/sync_setup_flow.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/webui/options/sync_setup_handler.h"
#include "chrome/common/net/gaia/google_service_auth_error.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/browser_with_test_window_test.h"
#include "chrome/test/test_browser_window.h"
#include "chrome/test/testing_profile.h"
#include "testing/gtest/include/gtest/gtest.h"

static const char kTestUser[] = "chrome.p13n.test@gmail.com";
static const char kTestPassword[] = "passwd";
static const char kTestCaptcha[] = "pizzamyheart";
static const char kTestCaptchaUrl[] = "http://pizzamyheart/";

typedef GoogleServiceAuthError AuthError;

class MockSyncSetupHandler : public SyncSetupHandler {
 public:
  MockSyncSetupHandler() {}

  // SyncSetupFlowHandler implementation.
  virtual void ShowGaiaLogin(const DictionaryValue& args) {}
  virtual void ShowGaiaSuccessAndClose() {}
  virtual void ShowGaiaSuccessAndSettingUp() {}
  virtual void ShowConfigure(const DictionaryValue& args) {}
  virtual void ShowPassphraseEntry(const DictionaryValue& args) {}
  virtual void ShowSettingUp() {}
  virtual void ShowSetupDone(const std::wstring& user) {
    flow()->OnDialogClosed("");
  }

  void CloseSetupUI() {
    ShowSetupDone(std::wstring());
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(MockSyncSetupHandler);
};

// A PSS subtype to inject.
class ProfileSyncServiceForWizardTest : public ProfileSyncService {
 public:
  ProfileSyncServiceForWizardTest(ProfileSyncFactory* factory, Profile* profile)
      : ProfileSyncService(factory, profile, ""),
        user_cancelled_dialog_(false),
        is_using_secondary_passphrase_(false) {
    RegisterPreferences();
    ResetTestStats();
  }

  virtual ~ProfileSyncServiceForWizardTest() {}

  virtual void OnUserSubmittedAuth(const std::string& username,
                                   const std::string& password,
                                   const std::string& captcha,
                                   const std::string& access_code) {
    username_ = username;
    password_ = password;
    captcha_ = captcha;
  }

  virtual void OnUserChoseDatatypes(bool sync_everything,
      const syncable::ModelTypeSet& chosen_types) {
    user_chose_data_types_ = true;
    chosen_data_types_ = chosen_types;
  }

  virtual void OnUserCancelledDialog() {
    user_cancelled_dialog_ = true;
  }

  virtual void SetPassphrase(const std::string& passphrase,
                             bool is_explicit,
                             bool is_creation) {
    passphrase_ = passphrase;
  }

  virtual string16 GetAuthenticatedUsername() const {
    return UTF8ToUTF16(username_);
  }

  virtual bool IsUsingSecondaryPassphrase() const {
    // The only value of |is_using_secondary_passphrase_| we current care about
    // is when it's true.
    if (!is_using_secondary_passphrase_)
      return ProfileSyncService::IsUsingSecondaryPassphrase();
    else
      return is_using_secondary_passphrase_;
  }

  void set_auth_state(const std::string& last_email,
                      const AuthError& error) {
    last_attempted_user_email_ = last_email;
    last_auth_error_ = error;
  }

  void set_last_auth_error(const AuthError& error) {
    last_auth_error_ = error;
  }

  void set_is_using_secondary_passphrase(bool secondary) {
    is_using_secondary_passphrase_ = secondary;
  }

  void set_passphrase_required_reason(
      sync_api::PassphraseRequiredReason reason) {
    passphrase_required_reason_ = reason;
  }

  void ResetTestStats() {
    username_.clear();
    password_.clear();
    captcha_.clear();
    user_cancelled_dialog_ = false;
    user_chose_data_types_ = false;
    keep_everything_synced_ = false;
    chosen_data_types_.clear();
  }

  std::string username_;
  std::string password_;
  std::string captcha_;
  bool user_cancelled_dialog_;
  bool user_chose_data_types_;
  bool keep_everything_synced_;
  bool is_using_secondary_passphrase_;
  syncable::ModelTypeSet chosen_data_types_;

  std::string passphrase_;

 private:
  DISALLOW_COPY_AND_ASSIGN(ProfileSyncServiceForWizardTest);
};

class TestingProfileWithSyncService : public TestingProfile {
 public:
  TestingProfileWithSyncService() {
    sync_service_.reset(new ProfileSyncServiceForWizardTest(&factory_, this));
  }

  virtual ProfileSyncService* GetProfileSyncService() {
    return sync_service_.get();
  }
 private:
  ProfileSyncFactoryMock factory_;
  scoped_ptr<ProfileSyncService> sync_service_;
};

// TODO(jhawkins): Subclass Browser (specifically, ShowOptionsTab) and inject it
// here to test the visibility of the Sync UI.
class SyncSetupWizardTest : public BrowserWithTestWindowTest {
 public:
  SyncSetupWizardTest()
      : wizard_(NULL),
        service_(NULL),
        flow_(NULL) {}
  virtual ~SyncSetupWizardTest() {}
  virtual void SetUp() {
    set_profile(new TestingProfileWithSyncService());
    profile()->CreateBookmarkModel(false);
    // Wait for the bookmarks model to load.
    profile()->BlockUntilBookmarkModelLoaded();
    set_browser(new Browser(Browser::TYPE_TABBED, profile()));
    browser()->set_window(window());
    BrowserList::SetLastActive(browser());
    service_ = static_cast<ProfileSyncServiceForWizardTest*>(
        profile()->GetProfileSyncService());
    wizard_.reset(new SyncSetupWizard(service_));
  }

  virtual void TearDown() {
    wizard_.reset();
    service_ = NULL;
    flow_ = NULL;
  }

 protected:
  void AttachSyncSetupHandler() {
    flow_ = wizard_->AttachSyncSetupHandler(&handler_);
  }

  void CompleteSetup() {
    // For a discrete run, we need to have ran through setup once.
    wizard_->Step(SyncSetupWizard::GAIA_LOGIN);
    AttachSyncSetupHandler();
    wizard_->Step(SyncSetupWizard::GAIA_SUCCESS);
    wizard_->Step(SyncSetupWizard::SYNC_EVERYTHING);
    wizard_->Step(SyncSetupWizard::SETTING_UP);
    wizard_->Step(SyncSetupWizard::DONE);
  }

  void CloseSetupUI() {
    handler_.CloseSetupUI();
  }

  scoped_ptr<SyncSetupWizard> wizard_;
  ProfileSyncServiceForWizardTest* service_;
  SyncSetupFlow* flow_;
  MockSyncSetupHandler handler_;
};

// See http://code.google.com/p/chromium/issues/detail?id=40715 for
// why we skip the below tests on OS X.  We don't use DISABLED_ as we
// would have to change the corresponding FRIEND_TEST() declarations.

#if defined(OS_MACOSX)
#define SKIP_TEST_ON_MACOSX() \
  do { LOG(WARNING) << "Test skipped on OS X"; return; } while (0)
#else
#define SKIP_TEST_ON_MACOSX() do {} while (0)
#endif

TEST_F(SyncSetupWizardTest, InitialStepLogin) {
  SKIP_TEST_ON_MACOSX();
  DictionaryValue dialog_args;
  SyncSetupFlow::GetArgsForGaiaLogin(service_, &dialog_args);
  std::string json_start_args;
  base::JSONWriter::Write(&dialog_args, false, &json_start_args);
  ListValue credentials;
  std::string auth = "{\"user\":\"";
  auth += std::string(kTestUser) + "\",\"pass\":\"";
  auth += std::string(kTestPassword) + "\",\"captcha\":\"";
  auth += std::string(kTestCaptcha) + "\",\"access_code\":\"";
  auth += std::string() + "\"}";
  credentials.Append(new StringValue(auth));

  EXPECT_FALSE(wizard_->IsVisible());
  EXPECT_EQ(static_cast<SyncSetupFlow*>(NULL), flow_);
  wizard_->Step(SyncSetupWizard::GAIA_LOGIN);
  AttachSyncSetupHandler();

  EXPECT_TRUE(wizard_->IsVisible());
  EXPECT_EQ(SyncSetupWizard::GAIA_LOGIN, flow_->current_state_);
  EXPECT_EQ(SyncSetupWizard::DONE, flow_->end_state_);
  EXPECT_EQ(json_start_args, flow_->dialog_start_args_);

  // Simulate the user submitting credentials.
  handler_.HandleSubmitAuth(&credentials);
  EXPECT_TRUE(wizard_->IsVisible());
  EXPECT_EQ(SyncSetupWizard::GAIA_LOGIN, flow_->current_state_);
  EXPECT_EQ(kTestUser, service_->username_);
  EXPECT_EQ(kTestPassword, service_->password_);
  EXPECT_EQ(kTestCaptcha, service_->captcha_);
  EXPECT_FALSE(service_->user_cancelled_dialog_);
  service_->ResetTestStats();

  // Simulate failed credentials.
  AuthError invalid_gaia(AuthError::INVALID_GAIA_CREDENTIALS);
  service_->set_auth_state(kTestUser, invalid_gaia);
  wizard_->Step(SyncSetupWizard::GAIA_LOGIN);
  EXPECT_TRUE(wizard_->IsVisible());
  EXPECT_EQ(SyncSetupWizard::GAIA_LOGIN, flow_->current_state_);
  dialog_args.Clear();
  SyncSetupFlow::GetArgsForGaiaLogin(service_, &dialog_args);
  EXPECT_EQ(4U, dialog_args.size());
  std::string actual_user;
  dialog_args.GetString("user", &actual_user);
  EXPECT_EQ(kTestUser, actual_user);
  int error = -1;
  dialog_args.GetInteger("error", &error);
  EXPECT_EQ(static_cast<int>(AuthError::INVALID_GAIA_CREDENTIALS), error);
  service_->set_auth_state(kTestUser, AuthError::None());

  // Simulate captcha.
  AuthError captcha_error(AuthError::FromCaptchaChallenge(
      std::string(), GURL(kTestCaptchaUrl), GURL()));
  service_->set_auth_state(kTestUser, captcha_error);
  wizard_->Step(SyncSetupWizard::GAIA_LOGIN);
  SyncSetupFlow::GetArgsForGaiaLogin(service_, &dialog_args);
  EXPECT_EQ(4U, dialog_args.size());
  std::string captcha_url;
  dialog_args.GetString("captchaUrl", &captcha_url);
  EXPECT_EQ(kTestCaptchaUrl, GURL(captcha_url).spec());
  error = -1;
  dialog_args.GetInteger("error", &error);
  EXPECT_EQ(static_cast<int>(AuthError::CAPTCHA_REQUIRED), error);
  service_->set_auth_state(kTestUser, AuthError::None());

  // Simulate success.
  wizard_->Step(SyncSetupWizard::GAIA_SUCCESS);
  EXPECT_TRUE(wizard_->IsVisible());
  wizard_->Step(SyncSetupWizard::SYNC_EVERYTHING);
  EXPECT_EQ(SyncSetupWizard::SYNC_EVERYTHING, flow_->current_state_);

  // That's all we're testing here, just move on to DONE.  We'll test the
  // "choose data types" scenarios elsewhere.
  wizard_->Step(SyncSetupWizard::SETTING_UP);  // No merge and sync.
  wizard_->Step(SyncSetupWizard::DONE);  // No merge and sync.
  EXPECT_FALSE(wizard_->IsVisible());
}

TEST_F(SyncSetupWizardTest, ChooseDataTypesSetsPrefs) {
  SKIP_TEST_ON_MACOSX();
  wizard_->Step(SyncSetupWizard::GAIA_LOGIN);
  AttachSyncSetupHandler();
  wizard_->Step(SyncSetupWizard::GAIA_SUCCESS);
  wizard_->Step(SyncSetupWizard::CONFIGURE);

  ListValue data_type_choices_value;
  std::string data_type_choices =
      "{\"keepEverythingSynced\":false,\"syncBookmarks\":true,"
      "\"syncPreferences\":true,\"syncThemes\":false,\"syncPasswords\":false,"
      "\"syncAutofill\":false,\"syncExtensions\":false,\"syncTypedUrls\":true,"
      "\"syncApps\":true,\"syncSessions\":false,\"usePassphrase\":false,"
      "\"encryptAllData\":false}";
  data_type_choices_value.Append(new StringValue(data_type_choices));

  // Simulate the user choosing data types; bookmarks, prefs, typed URLS, and
  // apps are on, the rest are off.
  handler_.HandleConfigure(&data_type_choices_value);
  EXPECT_TRUE(wizard_->IsVisible());
  EXPECT_FALSE(service_->keep_everything_synced_);
  EXPECT_EQ(1U, service_->chosen_data_types_.count(syncable::BOOKMARKS));
  EXPECT_EQ(1U, service_->chosen_data_types_.count(syncable::PREFERENCES));
  EXPECT_EQ(0U, service_->chosen_data_types_.count(syncable::THEMES));
  EXPECT_EQ(0U, service_->chosen_data_types_.count(syncable::PASSWORDS));
  EXPECT_EQ(0U, service_->chosen_data_types_.count(syncable::AUTOFILL));
  EXPECT_EQ(0U, service_->chosen_data_types_.count(syncable::EXTENSIONS));
  EXPECT_EQ(1U, service_->chosen_data_types_.count(syncable::TYPED_URLS));
  EXPECT_EQ(1U, service_->chosen_data_types_.count(syncable::APPS));

  CloseSetupUI();
}

TEST_F(SyncSetupWizardTest, EnterPassphraseRequired) {
  SKIP_TEST_ON_MACOSX();
  wizard_->Step(SyncSetupWizard::GAIA_LOGIN);
  AttachSyncSetupHandler();
  wizard_->Step(SyncSetupWizard::GAIA_SUCCESS);
  wizard_->Step(SyncSetupWizard::CONFIGURE);
  wizard_->Step(SyncSetupWizard::SETTING_UP);
  service_->set_passphrase_required_reason(sync_api::REASON_ENCRYPTION);
  wizard_->Step(SyncSetupWizard::ENTER_PASSPHRASE);
  EXPECT_EQ(SyncSetupWizard::ENTER_PASSPHRASE, flow_->current_state_);

  ListValue value;
  value.Append(new StringValue("{\"passphrase\":\"myPassphrase\","
                                "\"mode\":\"gaia\"}"));
  handler_.HandlePassphraseEntry(&value);
  EXPECT_EQ("myPassphrase", service_->passphrase_);
  CloseSetupUI();
}

TEST_F(SyncSetupWizardTest, DialogCancelled) {
  SKIP_TEST_ON_MACOSX();
  wizard_->Step(SyncSetupWizard::GAIA_LOGIN);
  AttachSyncSetupHandler();
  // Simulate the user closing the dialog.
  CloseSetupUI();
  EXPECT_FALSE(wizard_->IsVisible());
  EXPECT_TRUE(service_->user_cancelled_dialog_);
  EXPECT_EQ(std::string(), service_->username_);
  EXPECT_EQ(std::string(), service_->password_);

  wizard_->Step(SyncSetupWizard::GAIA_LOGIN);
  AttachSyncSetupHandler();
  EXPECT_TRUE(wizard_->IsVisible());
  wizard_->Step(SyncSetupWizard::GAIA_LOGIN);

  CloseSetupUI();
  EXPECT_FALSE(wizard_->IsVisible());
  EXPECT_TRUE(service_->user_cancelled_dialog_);
  EXPECT_EQ(std::string(), service_->username_);
  EXPECT_EQ(std::string(), service_->password_);
}

TEST_F(SyncSetupWizardTest, InvalidTransitions) {
  SKIP_TEST_ON_MACOSX();
  wizard_->Step(SyncSetupWizard::GAIA_SUCCESS);
  EXPECT_FALSE(wizard_->IsVisible());

  wizard_->Step(SyncSetupWizard::DONE);
  EXPECT_FALSE(wizard_->IsVisible());

  wizard_->Step(SyncSetupWizard::GAIA_LOGIN);
  AttachSyncSetupHandler();

  wizard_->Step(SyncSetupWizard::DONE);
  EXPECT_EQ(SyncSetupWizard::GAIA_LOGIN, flow_->current_state_);

  wizard_->Step(SyncSetupWizard::SETUP_ABORTED_BY_PENDING_CLEAR);
  EXPECT_EQ(SyncSetupWizard::GAIA_LOGIN, flow_->current_state_);

  wizard_->Step(SyncSetupWizard::GAIA_SUCCESS);
  wizard_->Step(SyncSetupWizard::SYNC_EVERYTHING);
  EXPECT_EQ(SyncSetupWizard::SYNC_EVERYTHING, flow_->current_state_);

  wizard_->Step(SyncSetupWizard::FATAL_ERROR);
  EXPECT_EQ(SyncSetupWizard::FATAL_ERROR, flow_->current_state_);
  CloseSetupUI();
}

TEST_F(SyncSetupWizardTest, FullSuccessfulRunSetsPref) {
  SKIP_TEST_ON_MACOSX();
  CompleteSetup();
  EXPECT_FALSE(wizard_->IsVisible());
  EXPECT_TRUE(service_->profile()->GetPrefs()->GetBoolean(
      prefs::kSyncHasSetupCompleted));
}

TEST_F(SyncSetupWizardTest, AbortedByPendingClear) {
  SKIP_TEST_ON_MACOSX();
  wizard_->Step(SyncSetupWizard::GAIA_LOGIN);
  AttachSyncSetupHandler();
  wizard_->Step(SyncSetupWizard::GAIA_SUCCESS);
  wizard_->Step(SyncSetupWizard::SYNC_EVERYTHING);
  wizard_->Step(SyncSetupWizard::SETUP_ABORTED_BY_PENDING_CLEAR);
  EXPECT_EQ(SyncSetupWizard::SETUP_ABORTED_BY_PENDING_CLEAR,
            flow_->current_state_);
  CloseSetupUI();
  EXPECT_FALSE(wizard_->IsVisible());
}

TEST_F(SyncSetupWizardTest, DiscreteRunChooseDataTypes) {
  SKIP_TEST_ON_MACOSX();

  CompleteSetup();

  wizard_->Step(SyncSetupWizard::CONFIGURE);
  AttachSyncSetupHandler();
  EXPECT_EQ(SyncSetupWizard::DONE, flow_->end_state_);

  wizard_->Step(SyncSetupWizard::SETTING_UP);
  wizard_->Step(SyncSetupWizard::DONE);
  EXPECT_FALSE(wizard_->IsVisible());
}

TEST_F(SyncSetupWizardTest, DiscreteRunChooseDataTypesAbortedByPendingClear) {
  SKIP_TEST_ON_MACOSX();

  CompleteSetup();

  wizard_->Step(SyncSetupWizard::CONFIGURE);
  AttachSyncSetupHandler();
  EXPECT_EQ(SyncSetupWizard::DONE, flow_->end_state_);
  wizard_->Step(SyncSetupWizard::SETUP_ABORTED_BY_PENDING_CLEAR);
  EXPECT_EQ(SyncSetupWizard::SETUP_ABORTED_BY_PENDING_CLEAR,
            flow_->current_state_);

  CloseSetupUI();
  EXPECT_FALSE(wizard_->IsVisible());
}

TEST_F(SyncSetupWizardTest, DiscreteRunGaiaLogin) {
  SKIP_TEST_ON_MACOSX();

  CompleteSetup();

  wizard_->Step(SyncSetupWizard::GAIA_LOGIN);
  AttachSyncSetupHandler();
  EXPECT_EQ(SyncSetupWizard::GAIA_SUCCESS, flow_->end_state_);

  AuthError invalid_gaia(AuthError::INVALID_GAIA_CREDENTIALS);
  service_->set_auth_state(kTestUser, invalid_gaia);
  wizard_->Step(SyncSetupWizard::GAIA_LOGIN);
  EXPECT_TRUE(wizard_->IsVisible());

  DictionaryValue dialog_args;
  SyncSetupFlow::GetArgsForGaiaLogin(service_, &dialog_args);
  EXPECT_EQ(4U, dialog_args.size());
  std::string actual_user;
  dialog_args.GetString("user", &actual_user);
  EXPECT_EQ(kTestUser, actual_user);
  int error = -1;
  dialog_args.GetInteger("error", &error);
  EXPECT_EQ(static_cast<int>(AuthError::INVALID_GAIA_CREDENTIALS), error);
  service_->set_auth_state(kTestUser, AuthError::None());

  wizard_->Step(SyncSetupWizard::GAIA_SUCCESS);
  CloseSetupUI();
}

TEST_F(SyncSetupWizardTest, NonFatalError) {
  SKIP_TEST_ON_MACOSX();

  CompleteSetup();

  // Set up the ENTER_PASSPHRASE case.
  service_->set_passphrase_required_reason(sync_api::REASON_ENCRYPTION);
  service_->set_is_using_secondary_passphrase(true);
  wizard_->Step(SyncSetupWizard::NONFATAL_ERROR);
  AttachSyncSetupHandler();
  EXPECT_EQ(SyncSetupWizard::ENTER_PASSPHRASE, flow_->current_state_);
  EXPECT_EQ(SyncSetupWizard::DONE, flow_->end_state_);
  CloseSetupUI();
  EXPECT_FALSE(wizard_->IsVisible());

  // Reset.
  service_->set_passphrase_required_reason(
      sync_api::REASON_PASSPHRASE_NOT_REQUIRED);
  service_->set_is_using_secondary_passphrase(false);

  // Test the various auth error states that lead to GAIA_LOGIN.

  service_->set_last_auth_error(
      AuthError(GoogleServiceAuthError::INVALID_GAIA_CREDENTIALS));
  wizard_->Step(SyncSetupWizard::NONFATAL_ERROR);
  AttachSyncSetupHandler();
  EXPECT_EQ(SyncSetupWizard::GAIA_LOGIN, flow_->current_state_);
  EXPECT_EQ(SyncSetupWizard::DONE, flow_->end_state_);
  CloseSetupUI();
  EXPECT_FALSE(wizard_->IsVisible());

  service_->set_last_auth_error(
      AuthError(GoogleServiceAuthError::CAPTCHA_REQUIRED));
  wizard_->Step(SyncSetupWizard::NONFATAL_ERROR);
  AttachSyncSetupHandler();
  EXPECT_EQ(SyncSetupWizard::GAIA_LOGIN, flow_->current_state_);
  EXPECT_EQ(SyncSetupWizard::DONE, flow_->end_state_);
  CloseSetupUI();
  EXPECT_FALSE(wizard_->IsVisible());

  service_->set_last_auth_error(
      AuthError(GoogleServiceAuthError::ACCOUNT_DELETED));
  wizard_->Step(SyncSetupWizard::NONFATAL_ERROR);
  AttachSyncSetupHandler();
  EXPECT_EQ(SyncSetupWizard::GAIA_LOGIN, flow_->current_state_);
  EXPECT_EQ(SyncSetupWizard::DONE, flow_->end_state_);
  CloseSetupUI();
  EXPECT_FALSE(wizard_->IsVisible());

  service_->set_last_auth_error(
      AuthError(GoogleServiceAuthError::ACCOUNT_DISABLED));
  wizard_->Step(SyncSetupWizard::NONFATAL_ERROR);
  AttachSyncSetupHandler();
  EXPECT_EQ(SyncSetupWizard::GAIA_LOGIN, flow_->current_state_);
  EXPECT_EQ(SyncSetupWizard::DONE, flow_->end_state_);
  CloseSetupUI();
  EXPECT_FALSE(wizard_->IsVisible());

  service_->set_last_auth_error(
      AuthError(GoogleServiceAuthError::SERVICE_UNAVAILABLE));
  wizard_->Step(SyncSetupWizard::NONFATAL_ERROR);
  AttachSyncSetupHandler();
  EXPECT_EQ(SyncSetupWizard::GAIA_LOGIN, flow_->current_state_);
  EXPECT_EQ(SyncSetupWizard::DONE, flow_->end_state_);
  CloseSetupUI();
  EXPECT_FALSE(wizard_->IsVisible());
}

#undef SKIP_TEST_ON_MACOSX
