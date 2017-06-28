// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/password_manager/core/browser/vote_uploader.h"

/*
#include <map>
#include <memory>
#include <set>
#include <utility>

#include "base/feature_list.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/message_loop/message_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/histogram_tester.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/user_action_tester.h"
#include "components/autofill/core/browser/autofill_manager.h"
#include "components/autofill/core/browser/proto/server.pb.h"
#include "components/autofill/core/browser/test_autofill_client.h"
#include "components/autofill/core/browser/test_autofill_driver.h"
#include "components/autofill/core/browser/test_personal_data_manager.h"
#include "components/autofill/core/common/autofill_pref_names.h"
#include "components/autofill/core/common/password_form.h"
#include "components/password_manager/core/browser/fake_form_fetcher.h"
#include "components/password_manager/core/browser/password_manager.h"
#include "components/password_manager/core/browser/password_manager_driver.h"
#include
"components/password_manager/core/browser/password_manager_metrics_util.h"
#include
"components/password_manager/core/browser/password_manager_test_utils.h"
#include "components/password_manager/core/browser/statistics_table.h"
#include "components/password_manager/core/browser/stub_credentials_filter.h"
#include "components/password_manager/core/browser/stub_form_saver.h"
#include
"components/password_manager/core/browser/stub_password_manager_client.h"
#include
"components/password_manager/core/browser/stub_password_manager_driver.h"
#include "components/password_manager/core/common/password_manager_features.h"
#include "components/password_manager/core/common/password_manager_pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"
#include "url/origin.h"

using autofill::FieldPropertiesFlags;
using autofill::FieldPropertiesMask;
using autofill::PasswordForm;
using autofill::PossibleUsernamePair;
using base::ASCIIToUTF16;
using ::testing::_;
using ::testing::IsEmpty;
using ::testing::Mock;
using ::testing::NiceMock;
using ::testing::Pointee;
using ::testing::Return;
using ::testing::SaveArg;
using ::testing::SaveArgPointee;
using ::testing::UnorderedElementsAre;
using ::testing::WithArg;
*/

namespace password_manager {

namespace {

MATCHER_P3(CheckUploadedAutofillTypesAndSignature,
           form_signature,
           expected_types,
           expect_generation_vote,
           "Unexpected autofill types or form signature") {
  if (form_signature != arg.FormSignatureAsStr()) {
    // Unexpected form's signature.
    ADD_FAILURE() << "Expected form signature is " << form_signature
                  << ", but found " << arg.FormSignatureAsStr();
    return false;
  }
  bool found_generation_vote = false;
  for (const auto& field : arg) {
    if (field->possible_types().size() > 1) {
      ADD_FAILURE() << field->name << " field has several possible types";
      return false;
    }

    found_generation_vote |=
        field->generation_type() !=
        autofill::AutofillUploadContents::Field::NO_GENERATION;

    autofill::ServerFieldType expected_vote =
        expected_types.find(field->name) == expected_types.end()
            ? autofill::NO_SERVER_DATA
            : expected_types.find(field->name)->second;
    autofill::ServerFieldType actual_vote =
        field->possible_types().empty() ? autofill::NO_SERVER_DATA
                                        : *field->possible_types().begin();
    if (expected_vote != actual_vote) {
      ADD_FAILURE() << field->name << " field: expected vote " << expected_vote
                    << ", but found " << actual_vote;
      return false;
    }
  }
  EXPECT_EQ(expect_generation_vote, found_generation_vote);
  return true;
}

MATCHER_P3(CheckUploadedGenerationTypesAndSignature,
           form_signature,
           expected_generation_types,
           generated_password_changed,
           "Unexpected generation types or form signature") {
  if (form_signature != arg.FormSignatureAsStr()) {
    // Unexpected form's signature.
    ADD_FAILURE() << "Expected form signature is " << form_signature
                  << ", but found " << arg.FormSignatureAsStr();
    return false;
  }
  for (const auto& field : arg) {
    if (expected_generation_types.find(field->name) ==
        expected_generation_types.end()) {
      if (field->generation_type() !=
          autofill::AutofillUploadContents::Field::NO_GENERATION) {
        // Unexpected generation type.
        ADD_FAILURE() << "Expected no generation type for the field "
                      << field->name << ", but found "
                      << field->generation_type();
        return false;
      }
    } else {
      if (expected_generation_types.find(field->name)->second !=
          field->generation_type()) {
        // Wrong generation type.
        ADD_FAILURE() << "Expected generation type for the field "
                      << field->name << " is "
                      << expected_generation_types.find(field->name)->second
                      << ", but found " << field->generation_type();
        return false;
      }

      if (field->generation_type() !=
          autofill::AutofillUploadContents::Field::IGNORED_GENERATION_POPUP) {
        EXPECT_EQ(generated_password_changed,
                  field->generated_password_changed());
      }
    }
  }
  return true;
}

MATCHER_P2(CheckUploadedFormClassifierVote,
           found_generation_element,
           generation_element,
           "Wrong form classifier votes") {
  for (const auto& field : arg) {
    if (found_generation_element && field->name == generation_element) {
      EXPECT_EQ(field->form_classifier_outcome(),
                autofill::AutofillUploadContents::Field::GENERATION_ELEMENT);
    } else {
      EXPECT_EQ(
          field->form_classifier_outcome(),
          autofill::AutofillUploadContents::Field::NON_GENERATION_ELEMENT);
    }
  }
  return true;
}

MATCHER_P(CheckFieldPropertiesMasksUpload,
          expected_field_properties,
          "Wrong field properties flags") {
  for (const auto& field : arg) {
    autofill::FieldPropertiesMask expected_mask =
        expected_field_properties.find(field->name) !=
                expected_field_properties.end()
            ? FieldPropertiesFlags::USER_TYPED
            : 0;
    if (field->properties_mask != expected_mask) {
      ADD_FAILURE() << "Wrong field properties flags for field " << field->name
                    << ": expected mask " << expected_mask << ", but found "
                    << field->properties_mask;
      return false;
    }
  }
  return true;
}

class MockAutofillDownloadManager : public autofill::AutofillDownloadManager {
 public:
  MockAutofillDownloadManager(
      autofill::AutofillDriver* driver,
      autofill::AutofillDownloadManager::Observer* observer)
      : AutofillDownloadManager(driver, observer) {}

  MOCK_METHOD5(StartUploadRequest,
               bool(const autofill::FormStructure&,
                    bool,
                    const autofill::ServerFieldTypeSet&,
                    const std::string&,
                    bool));

 private:
  DISALLOW_COPY_AND_ASSIGN(MockAutofillDownloadManager);
};

class MockAutofillManager : public autofill::AutofillManager {
 public:
  MockAutofillManager(autofill::AutofillDriver* driver,
                      autofill::AutofillClient* client,
                      autofill::PersonalDataManager* data_manager)
      : AutofillManager(driver, client, data_manager) {}

  void SetDownloadManager(autofill::AutofillDownloadManager* manager) {
    set_download_manager(manager);
  }

  // Workaround for std::unique_ptr<> lacking a copy constructor.
  void StartUploadProcess(std::unique_ptr<FormStructure> form_structure,
                          const base::TimeTicks& timestamp,
                          bool observed_submission) {
    StartUploadProcessPtr(form_structure.release(), timestamp,
                          observed_submission);
  }

  MOCK_METHOD3(StartUploadProcessPtr,
               void(FormStructure*, const base::TimeTicks&, bool));

 private:
  DISALLOW_COPY_AND_ASSIGN(MockAutofillManager);
};

class MockPasswordManagerDriver : public StubPasswordManagerDriver {
 public:
  MockPasswordManagerDriver()
      : mock_autofill_manager_(&test_autofill_driver_,
                               &test_autofill_client_,
                               &test_personal_data_manager_) {
    std::unique_ptr<TestingPrefServiceSimple> prefs(
        new TestingPrefServiceSimple());
    prefs->registry()->RegisterBooleanPref(autofill::prefs::kAutofillEnabled,
                                           true);
    test_autofill_client_.SetPrefs(std::move(prefs));
    mock_autofill_download_manager_ = new MockAutofillDownloadManager(
        &test_autofill_driver_, &mock_autofill_manager_);
    // AutofillManager takes ownership of |mock_autofill_download_manager_|.
    mock_autofill_manager_.SetDownloadManager(mock_autofill_download_manager_);
  }

  ~MockPasswordManagerDriver() {}

  MOCK_METHOD1(FillPasswordForm, void(const autofill::PasswordFormFillData&));
  MOCK_METHOD1(ShowInitialPasswordAccountSuggestions,
               void(const autofill::PasswordFormFillData&));
  MOCK_METHOD1(AllowPasswordGenerationForForm,
               void(const autofill::PasswordForm&));

  MockAutofillManager* mock_autofill_manager() {
    return &mock_autofill_manager_;
  }

  MockAutofillDownloadManager* mock_autofill_download_manager() {
    return mock_autofill_download_manager_;
  }

 private:
  autofill::TestAutofillDriver test_autofill_driver_;
  autofill::TestAutofillClient test_autofill_client_;
  autofill::TestPersonalDataManager test_personal_data_manager_;
  MockAutofillDownloadManager* mock_autofill_download_manager_;

  NiceMock<MockAutofillManager> mock_autofill_manager_;
};

class TestPasswordManagerClient : public StubPasswordManagerClient {
 public:
  TestPasswordManagerClient()
      : driver_(new NiceMock<MockPasswordManagerDriver>) {
    prefs_.registry()->RegisterBooleanPref(prefs::kCredentialsEnableService,
                                           true);
  }

  PrefService* GetPrefs() override { return &prefs_; }

  MockPasswordManagerDriver* mock_driver() { return driver_.get(); }

  base::WeakPtr<PasswordManagerDriver> driver() { return driver_->AsWeakPtr(); }

  autofill::AutofillManager* GetAutofillManagerForMainFrame() override {
    return mock_driver()->mock_autofill_manager();
  }

  void KillDriver() { driver_.reset(); }

 private:
  TestingPrefServiceSimple prefs_;
  std::unique_ptr<MockPasswordManagerDriver> driver_;
};

}  // namespace

class PasswordFormManagerTest : public testing::Test {
 public:
  PasswordFormManagerTest() { fake_form_fetcher_.Fetch(); }

  void SetUp() override {
    observed_form_.origin = GURL("http://accounts.google.com/a/LoginAuth");
    observed_form_.action = GURL("http://accounts.google.com/a/Login");
    observed_form_.username_element = ASCIIToUTF16("Email");
    observed_form_.password_element = ASCIIToUTF16("Passwd");
    observed_form_.submit_element = ASCIIToUTF16("signIn");
    observed_form_.signon_realm = "http://accounts.google.com";

    saved_match_ = observed_form_;
    saved_match_.origin = GURL("http://accounts.google.com/a/ServiceLoginAuth");
    saved_match_.action = GURL("http://accounts.google.com/a/ServiceLogin");
    saved_match_.preferred = true;
    saved_match_.username_value = ASCIIToUTF16("test@gmail.com");
    saved_match_.password_value = ASCIIToUTF16("test1");
    saved_match_.other_possible_usernames.push_back(PossibleUsernamePair(
        ASCIIToUTF16("test2@gmail.com"), ASCIIToUTF16("full_name")));

    psl_saved_match_ = saved_match_;
    psl_saved_match_.is_public_suffix_match = true;
    psl_saved_match_.origin =
        GURL("http://m.accounts.google.com/a/ServiceLoginAuth");
    psl_saved_match_.action = GURL("http://m.accounts.google.com/a/Login");
    psl_saved_match_.signon_realm = "http://m.accounts.google.com";

    autofill::FormFieldData field;
    field.label = ASCIIToUTF16("full_name");
    field.name = ASCIIToUTF16("full_name");
    field.form_control_type = "text";
    saved_match_.form_data.fields.push_back(field);

    field.label = ASCIIToUTF16("Email");
    field.name = ASCIIToUTF16("Email");
    field.form_control_type = "text";
    saved_match_.form_data.fields.push_back(field);

    field.label = ASCIIToUTF16("password");
    field.name = ASCIIToUTF16("Passwd");
    field.form_control_type = "password";
    saved_match_.form_data.fields.push_back(field);

    password_manager_.reset(new PasswordManager(&client_));
    form_manager_.reset(new PasswordFormManager(
        password_manager_.get(), &client_, client_.driver(), observed_form_,
        base::MakeUnique<NiceMock<MockFormSaver>>(), &fake_form_fetcher_));
  }

  // Save saved_match() for observed_form() where |observed_form_data|,
  // |times_used|, and |status| are used to overwrite the default values for
  // observed_form(). |field_type| is the upload that we expect from saving,
  // with nullptr meaning no upload expected.
  void AccountCreationUploadTest(const autofill::FormData& observed_form_data,
                                 int times_used,
                                 PasswordForm::GenerationUploadStatus status,
                                 const autofill::ServerFieldType* field_type) {
    PasswordForm form(*observed_form());

    form.form_data = observed_form_data;

    FakeFormFetcher fetcher;
    fetcher.Fetch();
    PasswordFormManager form_manager(
        password_manager(), client(), client()->driver(), form,
        base::MakeUnique<NiceMock<MockFormSaver>>(), &fetcher);
    PasswordForm match = CreateSavedMatch(false);
    match.generation_upload_status = status;
    match.times_used = times_used;

    PasswordForm form_to_save(form);
    form_to_save.preferred = true;
    form_to_save.username_element = ASCIIToUTF16("observed-username-field");
    form_to_save.username_value = match.username_value;
    form_to_save.password_value = match.password_value;

    fetcher.SetNonFederated({&match}, 0u);
    std::string expected_login_signature;
    autofill::FormStructure observed_structure(observed_form_data);
    autofill::FormStructure pending_structure(saved_match()->form_data);
    if (observed_structure.FormSignatureAsStr() !=
            pending_structure.FormSignatureAsStr() &&
        times_used == 0) {
      expected_login_signature = observed_structure.FormSignatureAsStr();
    }
    autofill::ServerFieldTypeSet expected_available_field_types;
    FieldTypeMap expected_types;
    expected_types[ASCIIToUTF16("full_name")] = autofill::UNKNOWN_TYPE;
    expected_types[match.username_element] = autofill::UNKNOWN_TYPE;

    bool expect_generation_vote = false;
    if (field_type) {
      // Show the password generation popup to check that the generation vote
      // would be ignored.
      form_manager.set_generation_element(saved_match()->password_element);
      form_manager.set_generation_popup_was_shown(true);
      expect_generation_vote =
          *field_type != autofill::ACCOUNT_CREATION_PASSWORD;

      expected_available_field_types.insert(*field_type);
      expected_types[saved_match()->password_element] = *field_type;
    }

    if (field_type) {
      EXPECT_CALL(
          *client()->mock_driver()->mock_autofill_download_manager(),
          StartUploadRequest(CheckUploadedAutofillTypesAndSignature(
                                 pending_structure.FormSignatureAsStr(),
                                 expected_types, expect_generation_vote),
                             false, expected_available_field_types,
                             expected_login_signature, true));
    } else {
      EXPECT_CALL(*client()->mock_driver()->mock_autofill_download_manager(),
                  StartUploadRequest(_, _, _, _, _))
          .Times(0);
    }
    form_manager.ProvisionallySave(
        form_to_save, PasswordFormManager::IGNORE_OTHER_POSSIBLE_USERNAMES);
    form_manager.Save();
    Mock::VerifyAndClearExpectations(
        client()->mock_driver()->mock_autofill_download_manager());
  }

  // Test upload votes on change password forms. |field_type| is a vote that we
  // expect to be uploaded.
  void ChangePasswordUploadTest(autofill::ServerFieldType field_type,
                                bool has_confirmation_field) {
    SCOPED_TRACE(testing::Message()
                 << "field_type=" << field_type
                 << " has_confirmation_field=" << has_confirmation_field);

    // |observed_form_| should have |form_data| in order to be uploaded.
    observed_form()->form_data = saved_match()->form_data;
    // Turn |observed_form_| and  into change password form.
    observed_form()->new_password_element = ASCIIToUTF16("NewPasswd");
    observed_form()->confirmation_password_element = ASCIIToUTF16("ConfPwd");
    autofill::FormFieldData field;
    field.label = ASCIIToUTF16("NewPasswd");
    field.name = ASCIIToUTF16("NewPasswd");
    field.form_control_type = "password";
    observed_form()->form_data.fields.push_back(field);
    autofill::FormFieldData empty_field;
    observed_form()->form_data.fields.push_back(empty_field);
    if (has_confirmation_field) {
      field.label = ASCIIToUTF16("ConfPwd");
      field.name = ASCIIToUTF16("ConfPwd");
      field.form_control_type = "password";
      observed_form()->form_data.fields.push_back(field);
    }

    FakeFormFetcher fetcher;
    fetcher.Fetch();
    PasswordFormManager form_manager(
        password_manager(), client(), client()->driver(), *observed_form(),
        base::MakeUnique<NiceMock<MockFormSaver>>(), &fetcher);
    fetcher.SetNonFederated({saved_match()}, 0u);

    // User submits current and new credentials to the observed form.
    PasswordForm submitted_form(*observed_form());
    // credentials.username_element.clear();
    submitted_form.username_value = saved_match()->username_value;
    submitted_form.password_value = saved_match()->password_value;
    submitted_form.new_password_value = ASCIIToUTF16("test2");
    submitted_form.preferred = true;
    form_manager.ProvisionallySave(
        submitted_form, PasswordFormManager::IGNORE_OTHER_POSSIBLE_USERNAMES);

    // Successful login. The PasswordManager would instruct PasswordFormManager
    // to update.
    EXPECT_FALSE(form_manager.IsNewLogin());
    EXPECT_FALSE(
        form_manager.is_possible_change_password_form_without_username());

    // By now, the PasswordFormManager should have promoted the new password
    // value already to be the current password, and should no longer maintain
    // any info about the new password value.
    EXPECT_EQ(submitted_form.new_password_value,
              form_manager.pending_credentials().password_value);
    EXPECT_TRUE(form_manager.pending_credentials().new_password_value.empty());

    std::map<base::string16, autofill::ServerFieldType> expected_types;
    expected_types[ASCIIToUTF16("full_name")] = autofill::UNKNOWN_TYPE;
    expected_types[observed_form_.username_element] = autofill::UNKNOWN_TYPE;
    expected_types[observed_form_.password_element] = autofill::PASSWORD;
    expected_types[observed_form_.new_password_element] = field_type;
    expected_types[base::string16()] = autofill::UNKNOWN_TYPE;

    autofill::ServerFieldTypeSet expected_available_field_types;
    expected_available_field_types.insert(autofill::PASSWORD);
    expected_available_field_types.insert(field_type);
    if (has_confirmation_field) {
      expected_types[observed_form_.confirmation_password_element] =
          autofill::CONFIRMATION_PASSWORD;
      expected_available_field_types.insert(autofill::CONFIRMATION_PASSWORD);
    }

    std::string observed_form_signature =
        autofill::FormStructure(observed_form()->form_data)
            .FormSignatureAsStr();

    std::string expected_login_signature;
    if (field_type == autofill::NEW_PASSWORD) {
      autofill::FormStructure pending_structure(saved_match()->form_data);
      expected_login_signature = pending_structure.FormSignatureAsStr();
    }
    EXPECT_CALL(*client()->mock_driver()->mock_autofill_download_manager(),
                StartUploadRequest(CheckUploadedAutofillTypesAndSignature(
                                       observed_form_signature, expected_types,
                                       false /* expect_generation_vote */),
                                   false, expected_available_field_types,
                                   expected_login_signature, true));

    switch (field_type) {
      case autofill::NEW_PASSWORD:
        form_manager.Update(*saved_match());
        break;
      case autofill::PROBABLY_NEW_PASSWORD:
        form_manager.OnNoInteraction(true /* it is an update */);
        break;
      case autofill::NOT_NEW_PASSWORD:
        form_manager.OnNopeUpdateClicked();
        break;
      default:
        NOTREACHED();
    }
    Mock::VerifyAndClearExpectations(
        client()->mock_driver()->mock_autofill_download_manager());
  }

  autofill::AutofillUploadContents::Field::PasswordGenerationType
  GetExpectedPasswordGenerationType(bool is_manual_generation,
                                    bool is_change_password_form,
                                    bool has_generated_password) {
    if (!has_generated_password)
      return autofill::AutofillUploadContents::Field::IGNORED_GENERATION_POPUP;

    if (is_manual_generation) {
      if (is_change_password_form) {
        return autofill::AutofillUploadContents::Field::
            MANUALLY_TRIGGERED_GENERATION_ON_CHANGE_PASSWORD_FORM;
      } else {
        return autofill::AutofillUploadContents::Field::
            MANUALLY_TRIGGERED_GENERATION_ON_SIGN_UP_FORM;
      }
    } else {
      if (is_change_password_form) {
        return autofill::AutofillUploadContents::Field::
            AUTOMATICALLY_TRIGGERED_GENERATION_ON_CHANGE_PASSWORD_FORM;
      } else {
        return autofill::AutofillUploadContents::Field::
            AUTOMATICALLY_TRIGGERED_GENERATION_ON_SIGN_UP_FORM;
      }
    }
  }

  // The user types username and generates password on SignUp or change password
  // form. The password generation might be triggered automatically or manually.
  // This function checks that correct vote is uploaded on server. The vote must
  // be uploaded regardless of the user's interaction with the prompt.
  void GeneratedVoteUploadTest(bool is_manual_generation,
                               bool is_change_password_form,
                               bool has_generated_password,
                               bool generated_password_changed,
                               SavePromptInteraction interaction) {
    SCOPED_TRACE(testing::Message()
                 << "is_manual_generation=" << is_manual_generation
                 << " is_change_password_form=" << is_change_password_form
                 << " has_generated_password=" << has_generated_password
                 << " generated_password_changed=" << generated_password_changed
                 << " interaction=" << interaction);
    PasswordForm form(*observed_form());
    form.form_data = saved_match()->form_data;

    if (is_change_password_form) {
      // Turn |form| to a change password form.
      form.new_password_element = ASCIIToUTF16("NewPasswd");

      autofill::FormFieldData field;
      field.label = ASCIIToUTF16("password");
      field.name = ASCIIToUTF16("NewPasswd");
      field.form_control_type = "password";
      form.form_data.fields.push_back(field);
    }

    // Create submitted form.
    PasswordForm submitted_form(form);
    submitted_form.preferred = true;
    submitted_form.username_value = saved_match()->username_value;
    submitted_form.password_value = saved_match()->password_value;

    if (is_change_password_form) {
      submitted_form.new_password_value =
          saved_match()->password_value + ASCIIToUTF16("1");
    }

    FakeFormFetcher fetcher;
    fetcher.Fetch();
    PasswordFormManager form_manager(
        password_manager(), client(), client()->driver(), form,
        base::MakeUnique<NiceMock<MockFormSaver>>(), &fetcher);

    fetcher.SetNonFederated(std::vector<const PasswordForm*>(), 0u);

    autofill::ServerFieldTypeSet expected_available_field_types;
    // Don't send autofill votes if the user didn't press "Save" button.
    if (interaction == SAVE)
      expected_available_field_types.insert(autofill::PASSWORD);

    form_manager.set_is_manual_generation(is_manual_generation);
    base::string16 generation_element = is_change_password_form
                                            ? form.new_password_element
                                            : form.password_element;
    form_manager.set_generation_element(generation_element);
    form_manager.set_generation_popup_was_shown(true);
    form_manager.SetHasGeneratedPassword(has_generated_password);
    if (has_generated_password)
      form_manager.set_generated_password_changed(generated_password_changed);

    // Figure out expected generation event type.
    autofill::AutofillUploadContents::Field::PasswordGenerationType
        expected_generation_type = GetExpectedPasswordGenerationType(
            is_manual_generation, is_change_password_form,
            has_generated_password);
    std::map<base::string16,
             autofill::AutofillUploadContents::Field::PasswordGenerationType>
        expected_generation_types;
    expected_generation_types[generation_element] = expected_generation_type;

    autofill::FormStructure form_structure(submitted_form.form_data);

    EXPECT_CALL(
        *client()->mock_driver()->mock_autofill_download_manager(),
        StartUploadRequest(
            CheckUploadedGenerationTypesAndSignature(
                form_structure.FormSignatureAsStr(), expected_generation_types,
                generated_password_changed),
            false, expected_available_field_types, std::string(), true));

    form_manager.ProvisionallySave(
        submitted_form, PasswordFormManager::IGNORE_OTHER_POSSIBLE_USERNAMES);
    switch (interaction) {
      case SAVE:
        form_manager.Save();
        break;
      case NEVER:
        form_manager.OnNeverClicked();
        break;
      case NO_INTERACTION:
        form_manager.OnNoInteraction(false /* not an update prompt*/);
        break;
    }
    Mock::VerifyAndClearExpectations(
        client()->mock_driver()->mock_autofill_download_manager());
  }

  PasswordForm* observed_form() { return &observed_form_; }
  PasswordForm* saved_match() { return &saved_match_; }
  PasswordForm* psl_saved_match() { return &psl_saved_match_; }
  PasswordForm CreateSavedMatch(bool blacklisted) {
    PasswordForm match = saved_match_;
    match.blacklisted_by_user = blacklisted;
    return match;
  }

  TestPasswordManagerClient* client() { return &client_; }

  PasswordManager* password_manager() { return password_manager_.get(); }

  PasswordFormManager* form_manager() { return form_manager_.get(); }

  FakeFormFetcher* fake_form_fetcher() { return &fake_form_fetcher_; }

  // To spare typing for PasswordFormManager instances which need no driver.
  const base::WeakPtr<PasswordManagerDriver> kNoDriver;

 protected:
  enum class SimulatedManagerAction { NONE, AUTOFILLED, OFFERED, OFFERED_PSL };
  enum class SimulatedSubmitResult { NONE, PASSED, FAILED };
  enum class SuppressedFormType { HTTPS, PSL_MATCH, SAME_ORGANIZATION_NAME };

  PasswordForm CreateSuppressedForm(SuppressedFormType suppression_type,
                                    const char* username,
                                    const char* password,
                                    PasswordForm::Type manual_or_generated) {
    PasswordForm form = *saved_match();
    switch (suppression_type) {
      case SuppressedFormType::HTTPS:
        form.origin = GURL("https://accounts.google.com/a/LoginAuth");
        form.signon_realm = "https://accounts.google.com/";
        break;
      case SuppressedFormType::PSL_MATCH:
        form.origin = GURL("http://other.google.com/");
        form.signon_realm = "http://other.google.com/";
        break;
      case SuppressedFormType::SAME_ORGANIZATION_NAME:
        form.origin = GURL("https://may-or-may-not-be.google.appspot.com/");
        form.signon_realm = "https://may-or-may-not-be.google.appspot.com/";
        break;
    }
    form.type = manual_or_generated;
    form.username_value = ASCIIToUTF16(username);
    form.password_value = ASCIIToUTF16(password);
    return form;
  }

  void SimulateActionsOnHTTPObservedForm(
      FakeFormFetcher* fetcher,
      SimulatedManagerAction manager_action,
      SimulatedSubmitResult submit_result,
      const char* filled_username,
      const char* filled_password,
      const char* submitted_password = nullptr) {
    PasswordFormManager form_manager(
        password_manager(), client(), client()->driver(), *observed_form(),
        base::MakeUnique<NiceMock<MockFormSaver>>(), fetcher);

    EXPECT_CALL(*client()->mock_driver()->mock_autofill_download_manager(),
                StartUploadRequest(_, _, _, _, _))
        .Times(::testing::AnyNumber());

    PasswordForm http_stored_form = *saved_match();
    http_stored_form.username_value = base::ASCIIToUTF16(filled_username);
    http_stored_form.password_value = base::ASCIIToUTF16(filled_password);
    if (manager_action == SimulatedManagerAction::OFFERED_PSL)
      http_stored_form.is_public_suffix_match = true;

    std::vector<const PasswordForm*> matches;
    if (manager_action != SimulatedManagerAction::NONE)
      matches.push_back(&http_stored_form);

    // Extra mile: kChoose is only recorded if there were multiple
    // logins available and the preferred one was changed.
    PasswordForm http_stored_form2 = http_stored_form;
    if (manager_action == SimulatedManagerAction::OFFERED) {
      http_stored_form.preferred = false;
      http_stored_form2.username_value = ASCIIToUTF16("user-other@gmail.com");
      matches.push_back(&http_stored_form2);
    }

    fetcher->Fetch();
    fetcher->SetNonFederated(matches, 0u);

    if (submit_result != SimulatedSubmitResult::NONE) {
      PasswordForm submitted_form(*observed_form());
      submitted_form.preferred = true;
      submitted_form.username_value = base::ASCIIToUTF16(filled_username);
      submitted_form.password_value =
          submitted_password ? base::ASCIIToUTF16(submitted_password)
                             : base::ASCIIToUTF16(filled_password);

      form_manager.ProvisionallySave(
          submitted_form, PasswordFormManager::IGNORE_OTHER_POSSIBLE_USERNAMES);
      if (submit_result == SimulatedSubmitResult::PASSED) {
        form_manager.LogSubmitPassed();
        form_manager.Save();
      } else {
        form_manager.LogSubmitFailed();
      }
    }
  }

 private:
  // Necessary for callbacks, and for TestAutofillDriver.
  base::MessageLoop message_loop_;

  PasswordForm observed_form_;
  PasswordForm saved_match_;
  PasswordForm psl_saved_match_;
  TestPasswordManagerClient client_;
  std::unique_ptr<PasswordManager> password_manager_;
  // Define |fake_form_fetcher_| before |form_manager_|, because the former
  // needs to outlive the latter.
  FakeFormFetcher fake_form_fetcher_;
  std::unique_ptr<PasswordFormManager> form_manager_;
};

TEST_F(PasswordFormManagerTest, UploadFormData_NewPassword) {
  // For newly saved passwords, upload a password vote for autofill::PASSWORD.
  // Don't vote for the username field yet.
  FakeFormFetcher fetcher;
  fetcher.Fetch();
  PasswordFormManager form_manager(
      password_manager(), client(), client()->driver(), *saved_match(),
      base::MakeUnique<NiceMock<MockFormSaver>>(), &fetcher);
  fetcher.SetNonFederated(std::vector<const PasswordForm*>(), 0u);

  PasswordForm form_to_save(*saved_match());
  form_to_save.preferred = true;
  form_to_save.username_value = ASCIIToUTF16("username");
  form_to_save.password_value = ASCIIToUTF16("1234");

  autofill::ServerFieldTypeSet expected_available_field_types = {
      autofill::PASSWORD};
  EXPECT_CALL(
      *client()->mock_driver()->mock_autofill_download_manager(),
      StartUploadRequest(_, false, expected_available_field_types, _, true));
  form_manager.ProvisionallySave(
      form_to_save, PasswordFormManager::IGNORE_OTHER_POSSIBLE_USERNAMES);
  form_manager.Save();
}

TEST_F(PasswordFormManagerTest, UploadFormData_NewPassword_Blacklist) {
  // Do not upload a vote if the user is blacklisting the form.
  FakeFormFetcher fetcher;
  fetcher.Fetch();
  PasswordFormManager blacklist_form_manager(
      password_manager(), client(), client()->driver(), *saved_match(),
      base::MakeUnique<NiceMock<MockFormSaver>>(), &fetcher);
  fetcher.SetNonFederated(std::vector<const PasswordForm*>(), 0u);

  autofill::ServerFieldTypeSet expected_available_field_types;
  expected_available_field_types.insert(autofill::USERNAME);
  expected_available_field_types.insert(autofill::PASSWORD);
  EXPECT_CALL(*client()->mock_driver()->mock_autofill_download_manager(),
              StartUploadRequest(_, _, expected_available_field_types, _, true))
      .Times(0);
  blacklist_form_manager.PermanentlyBlacklist();
}

TEST_F(PasswordFormManagerTest, UploadPasswordForm) {
  autofill::FormData observed_form_data;
  autofill::FormFieldData field;
  field.label = ASCIIToUTF16("Email");
  field.name = ASCIIToUTF16("observed-username-field");
  field.form_control_type = "text";
  observed_form_data.fields.push_back(field);

  field.label = ASCIIToUTF16("password");
  field.name = ASCIIToUTF16("password");
  field.form_control_type = "password";
  observed_form_data.fields.push_back(field);

  // Form data is different than saved form data, account creation signal should
  // be sent.
  autofill::ServerFieldType field_type = autofill::ACCOUNT_CREATION_PASSWORD;
  AccountCreationUploadTest(observed_form_data, 0, PasswordForm::NO_SIGNAL_SENT,
                            &field_type);

  // Non-zero times used will not upload since we only upload a positive signal
  // at most once.
  AccountCreationUploadTest(observed_form_data, 1, PasswordForm::NO_SIGNAL_SENT,
                            nullptr);

  // Same form data as saved match and POSITIVE_SIGNAL_SENT means there should
  // be a negative autofill ping sent.
  field_type = autofill::NOT_ACCOUNT_CREATION_PASSWORD;
  AccountCreationUploadTest(saved_match()->form_data, 2,
                            PasswordForm::POSITIVE_SIGNAL_SENT, &field_type);

  // For any other GenerationUplaodStatus, no autofill upload should occur
  // if the observed form data matches the saved form data.
  AccountCreationUploadTest(saved_match()->form_data, 3,
                            PasswordForm::NO_SIGNAL_SENT, nullptr);
  AccountCreationUploadTest(saved_match()->form_data, 3,
                            PasswordForm::NEGATIVE_SIGNAL_SENT, nullptr);
}

TEST_F(PasswordFormManagerTest, UploadChangePasswordForm) {
  autofill::ServerFieldType kChangePasswordVotes[] = {
      autofill::NEW_PASSWORD, autofill::PROBABLY_NEW_PASSWORD,
      autofill::NOT_NEW_PASSWORD};
  bool kFalseTrue[] = {false, true};
  for (autofill::ServerFieldType vote : kChangePasswordVotes) {
    for (bool has_confirmation_field : kFalseTrue)
      ChangePasswordUploadTest(vote, has_confirmation_field);
  }
}

TEST_F(PasswordFormManagerTest,
       TestUploadVotesForPasswordChangeFormsWithTwoFields) {
  // Turn |observed_form_| into change password form with only 2 fields: an old
  // password and a new password.
  observed_form()->username_element.clear();
  observed_form()->new_password_element = ASCIIToUTF16("NewPasswd");
  autofill::FormFieldData field;
  field.label = ASCIIToUTF16("password");
  field.name = ASCIIToUTF16("Passwd");
  field.form_control_type = "password";
  observed_form()->form_data.fields.push_back(field);

  field.label = ASCIIToUTF16("new password");
  field.name = ASCIIToUTF16("NewPasswd");
  observed_form()->form_data.fields.push_back(field);

  FakeFormFetcher fetcher;
  fetcher.Fetch();
  PasswordFormManager form_manager(password_manager(), client(),
                                   client()->driver(), *observed_form(),
                                   base::MakeUnique<MockFormSaver>(), &fetcher);
  fetcher.SetNonFederated({saved_match()}, 0u);

  // User submits current and new credentials to the observed form.
  PasswordForm submitted_form(*observed_form());
  submitted_form.password_value = saved_match()->password_value;
  submitted_form.new_password_value = ASCIIToUTF16("test2");
  submitted_form.preferred = true;
  form_manager.ProvisionallySave(
      submitted_form, PasswordFormManager::IGNORE_OTHER_POSSIBLE_USERNAMES);

  // Successful login. The PasswordManager would instruct PasswordFormManager
  // to update.
  EXPECT_FALSE(form_manager.IsNewLogin());
  EXPECT_TRUE(form_manager.is_possible_change_password_form_without_username());

  // By now, the PasswordFormManager should have promoted the new password
  // value already to be the current password, and should no longer maintain
  // any info about the new password value.
  EXPECT_EQ(submitted_form.new_password_value,
            form_manager.pending_credentials().password_value);
  EXPECT_TRUE(form_manager.pending_credentials().new_password_value.empty());

  std::map<base::string16, autofill::ServerFieldType> expected_types;
  expected_types[observed_form()->password_element] = autofill::PASSWORD;
  expected_types[observed_form()->new_password_element] =
      autofill::NEW_PASSWORD;

  autofill::ServerFieldTypeSet expected_available_field_types;
  expected_available_field_types.insert(autofill::PASSWORD);
  expected_available_field_types.insert(autofill::NEW_PASSWORD);

  std::string observed_form_signature =
      autofill::FormStructure(observed_form()->form_data).FormSignatureAsStr();

  std::string expected_login_signature =
      autofill::FormStructure(saved_match()->form_data).FormSignatureAsStr();

  EXPECT_CALL(*client()->mock_driver()->mock_autofill_download_manager(),
              StartUploadRequest(CheckUploadedAutofillTypesAndSignature(
                                     observed_form_signature, expected_types,
                                     false /* expect_generation_vote */),
                                 false, expected_available_field_types,
                                 expected_login_signature, true));

  form_manager.Update(*saved_match());
}

// Checks uploading a vote about the usage of the password generation popup.
TEST_F(PasswordFormManagerTest, GeneratedVoteUpload) {
  bool kFalseTrue[] = {false, true};
  SavePromptInteraction kSavePromptInterations[] = {SAVE, NEVER,
                                                    NO_INTERACTION};
  for (bool is_manual_generation : kFalseTrue) {
    for (bool is_change_password_form : kFalseTrue) {
      for (bool has_generated_password : kFalseTrue) {
        for (bool generated_password_changed : kFalseTrue) {
          for (SavePromptInteraction interaction : kSavePromptInterations) {
            GeneratedVoteUploadTest(is_manual_generation,
                                    is_change_password_form,
                                    has_generated_password,
                                    generated_password_changed, interaction);
          }
        }
      }
    }
  }
}

TEST_F(PasswordFormManagerTest, FormClassifierVoteUpload) {
  const bool kFalseTrue[] = {false, true};
  for (bool found_generation_element : kFalseTrue) {
    SCOPED_TRACE(testing::Message()
                 << "found_generation_element=" << found_generation_element);
    PasswordForm form(*observed_form());
    form.form_data = saved_match()->form_data;

    // Create submitted form.
    PasswordForm submitted_form(form);
    submitted_form.preferred = true;
    submitted_form.username_value = saved_match()->username_value;
    submitted_form.password_value = saved_match()->password_value;

    FakeFormFetcher fetcher;
    fetcher.Fetch();
    PasswordFormManager form_manager(
        password_manager(), client(), client()->driver(), form,
        base::MakeUnique<NiceMock<MockFormSaver>>(), &fetcher);
    base::string16 generation_element = form.password_element;
    if (found_generation_element)
      form_manager.SaveGenerationFieldDetectedByClassifier(generation_element);
    else
      form_manager.SaveGenerationFieldDetectedByClassifier(base::string16());

    fetcher.SetNonFederated(std::vector<const PasswordForm*>(), 0u);

    autofill::FormStructure form_structure(submitted_form.form_data);

    EXPECT_CALL(
        *client()->mock_driver()->mock_autofill_download_manager(),
        StartUploadRequest(CheckUploadedFormClassifierVote(
                               found_generation_element, generation_element),
                           false, _, _, true));

    form_manager.ProvisionallySave(
        submitted_form, PasswordFormManager::IGNORE_OTHER_POSSIBLE_USERNAMES);
    form_manager.Save();
  }
}

TEST_F(PasswordFormManagerTest, FieldPropertiesMasksUpload) {
  PasswordForm form(*observed_form());
  form.form_data = saved_match()->form_data;

  // Create submitted form.
  PasswordForm submitted_form(form);
  submitted_form.preferred = true;
  submitted_form.username_value = saved_match()->username_value;
  submitted_form.password_value = saved_match()->password_value;

  FakeFormFetcher fetcher;
  fetcher.Fetch();
  PasswordFormManager form_manager(
      password_manager(), client(), client()->driver(), form,
      base::MakeUnique<NiceMock<MockFormSaver>>(), &fetcher);
  fetcher.SetNonFederated(std::vector<const PasswordForm*>(), 0u);

  DCHECK_EQ(3U, form.form_data.fields.size());
  submitted_form.form_data.fields[1].properties_mask =
      FieldPropertiesFlags::USER_TYPED;
  submitted_form.form_data.fields[2].properties_mask =
      FieldPropertiesFlags::USER_TYPED;

  std::map<base::string16, autofill::FieldPropertiesMask>
      expected_field_properties;
  for (const autofill::FormFieldData& field : submitted_form.form_data.fields)
    if (field.properties_mask)
      expected_field_properties[field.name] = field.properties_mask;

  EXPECT_CALL(*client()->mock_driver()->mock_autofill_download_manager(),
              StartUploadRequest(
                  CheckFieldPropertiesMasksUpload(expected_field_properties),
                  false, _, _, true));
  form_manager.ProvisionallySave(
      submitted_form, PasswordFormManager::IGNORE_OTHER_POSSIBLE_USERNAMES);
  form_manager.Save();
}

TEST_F(PasswordFormManagerTest, ProbablyAccountCreationUpload) {
  PasswordForm form(*observed_form());
  form.form_data = saved_match()->form_data;

  FakeFormFetcher fetcher;
  fetcher.Fetch();
  PasswordFormManager form_manager(
      password_manager(), client(), client()->driver(), form,
      base::MakeUnique<NiceMock<MockFormSaver>>(), &fetcher);

  PasswordForm form_to_save(form);
  form_to_save.preferred = true;
  form_to_save.username_element = ASCIIToUTF16("observed-username-field");
  form_to_save.username_value = saved_match()->username_value;
  form_to_save.password_value = saved_match()->password_value;
  form_to_save.does_look_like_signup_form = true;

  fetcher.SetNonFederated(std::vector<const PasswordForm*>(), 0u);

  autofill::FormStructure pending_structure(form_to_save.form_data);
  autofill::ServerFieldTypeSet expected_available_field_types;
  std::map<base::string16, autofill::ServerFieldType> expected_types;
  expected_types[ASCIIToUTF16("full_name")] = autofill::UNKNOWN_TYPE;
  expected_types[saved_match()->username_element] = autofill::UNKNOWN_TYPE;
  expected_available_field_types.insert(
      autofill::PROBABLY_ACCOUNT_CREATION_PASSWORD);
  expected_types[saved_match()->password_element] =
      autofill::PROBABLY_ACCOUNT_CREATION_PASSWORD;

  EXPECT_CALL(*client()->mock_driver()->mock_autofill_download_manager(),
              StartUploadRequest(
                  CheckUploadedAutofillTypesAndSignature(
                      pending_structure.FormSignatureAsStr(), expected_types,
                      false /* expect_generation_vote */),
                  false, expected_available_field_types, std::string(), true));

  form_manager.ProvisionallySave(
      form_to_save, PasswordFormManager::IGNORE_OTHER_POSSIBLE_USERNAMES);
  form_manager.Save();
}

TEST_F(PasswordFormManagerTest, UploadUsernameCorrectionVote) {
  // Observed and saved forms have the same password, but different usernames.
  PasswordForm new_login(*observed_form());
  new_login.username_value = saved_match()->other_possible_usernames[0].first;
  new_login.password_value = saved_match()->password_value;

  fake_form_fetcher()->SetNonFederated({saved_match()}, 0u);
  form_manager()->ProvisionallySave(
      new_login, PasswordFormManager::IGNORE_OTHER_POSSIBLE_USERNAMES);
  // No match found (because usernames are different).
  EXPECT_TRUE(form_manager()->IsNewLogin());

  // Checks the username correction vote is saved.
  PasswordForm expected_username_vote(*saved_match());
  expected_username_vote.username_element =
      saved_match()->other_possible_usernames[0].second;

  // Checks the upload.
  autofill::ServerFieldTypeSet expected_available_field_types;
  expected_available_field_types.insert(autofill::USERNAME);
  expected_available_field_types.insert(autofill::ACCOUNT_CREATION_PASSWORD);

  FormStructure expected_upload(expected_username_vote.form_data);

  std::string expected_login_signature =
      FormStructure(form_manager()->observed_form().form_data)
          .FormSignatureAsStr();

  std::map<base::string16, autofill::ServerFieldType> expected_types;
  expected_types[expected_username_vote.username_element] = autofill::USERNAME;
  expected_types[expected_username_vote.password_element] =
      autofill::ACCOUNT_CREATION_PASSWORD;
  expected_types[ASCIIToUTF16("Email")] = autofill::UNKNOWN_TYPE;

  EXPECT_CALL(*client()->mock_driver()->mock_autofill_download_manager(),
              StartUploadRequest(CheckUploadedAutofillTypesAndSignature(
                                     expected_upload.FormSignatureAsStr(),
                                     expected_types, false),
                                 false, expected_available_field_types,
                                 expected_login_signature, true));
  form_manager()->Save();
}

TEST_F(PasswordFormManagerTest, UploadSignInForm_WithAutofillTypes) {
  // For newly saved passwords on a sign-in form, upload an autofill vote for a
  // username field and a autofill::PASSWORD vote for a password field.
  autofill::FormFieldData field;
  field.name = ASCIIToUTF16("Email");
  field.form_control_type = "text";
  observed_form()->form_data.fields.push_back(field);

  field.name = ASCIIToUTF16("Passwd");
  field.form_control_type = "password";
  observed_form()->form_data.fields.push_back(field);

  FakeFormFetcher fetcher;
  fetcher.Fetch();
  PasswordFormManager form_manager(
      password_manager(), client(), client()->driver(), *observed_form(),
      base::MakeUnique<NiceMock<MockFormSaver>>(), &fetcher);
  fetcher.SetNonFederated(std::vector<const PasswordForm*>(), 0u);

  PasswordForm form_to_save(*observed_form());
  form_to_save.preferred = true;
  form_to_save.username_value = ASCIIToUTF16("test@gmail.com");
  form_to_save.password_value = ASCIIToUTF16("password");

  std::unique_ptr<FormStructure> uploaded_form_structure;
  auto* mock_autofill_manager =
      client()->mock_driver()->mock_autofill_manager();
  EXPECT_CALL(*mock_autofill_manager, StartUploadProcessPtr(_, _, true))
      .WillOnce(WithArg<0>(SaveToUniquePtr(&uploaded_form_structure)));
  form_manager.ProvisionallySave(
      form_to_save, PasswordFormManager::IGNORE_OTHER_POSSIBLE_USERNAMES);
  form_manager.Save();

  ASSERT_EQ(2u, uploaded_form_structure->field_count());
  autofill::ServerFieldTypeSet expected_types = {autofill::PASSWORD};
  EXPECT_EQ(form_to_save.username_value,
            uploaded_form_structure->field(0)->value);
  EXPECT_EQ(expected_types,
            uploaded_form_structure->field(1)->possible_types());
}

// Checks that there is no upload on saving a password on a password form only
// with 1 field.
TEST_F(PasswordFormManagerTest, NoUploadsForSubmittedFormWithOnlyOneField) {
  autofill::FormFieldData field;
  field.name = ASCIIToUTF16("Passwd");
  field.form_control_type = "password";
  observed_form()->form_data.fields.push_back(field);

  FakeFormFetcher fetcher;
  fetcher.Fetch();
  PasswordFormManager form_manager(
      password_manager(), client(), client()->driver(), *observed_form(),
      base::MakeUnique<NiceMock<MockFormSaver>>(), &fetcher);
  fetcher.SetNonFederated(std::vector<const PasswordForm*>(), 0u);

  PasswordForm form_to_save(*observed_form());
  form_to_save.preferred = true;
  form_to_save.password_value = ASCIIToUTF16("password");

  auto* mock_autofill_manager =
      client()->mock_driver()->mock_autofill_manager();
  EXPECT_CALL(*mock_autofill_manager, StartUploadProcessPtr(_, _, _)).Times(0);
  form_manager.ProvisionallySave(
      form_to_save, PasswordFormManager::IGNORE_OTHER_POSSIBLE_USERNAMES);
  form_manager.Save();
}

}  // namespace password_manager
