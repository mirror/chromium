// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/password_manager/core/browser/vote_uploader_impl.h"

// TODO(vabr):
// * Write unittests for VU, consider if mocking is needed for PFM testing.
// * Address all TODO(vabr).
#include <map>
#include <utility>

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/string16.h"
#include "base/time/time.h"
#include "components/autofill/core/browser/autofill_manager.h"
#include "components/autofill/core/browser/form_structure.h"
#include "components/autofill/core/browser/proto/server.pb.h"
#include "components/autofill/core/common/form_data.h"
#include "components/password_manager/core/browser/browser_save_password_progress_logger.h"
#include "components/password_manager/core/browser/password_manager_client.h"
#include "components/password_manager/core/browser/password_manager_util.h"

using autofill::FormStructure;
using autofill::PasswordForm;

// Shorten the name to spare line breaks. The code provides enough context
// already.
using Logger = autofill::SavePasswordProgressLogger;

using FieldTypeMap = std::map<base::string16, autofill::ServerFieldType>;

namespace password_manager {

namespace {

// Sets autofill types of password and new password fields in |field_types|.
// |password_type| (the autofill type of new password field) should be equal to
// NEW_PASSWORD, PROBABLY_NEW_PASSWORD or NOT_NEW_PASSWORD. These values
// correspond to cases when the user confirmed password update, did nothing or
// declined to update password respectively.
void SetFieldLabelsOnUpdate(const autofill::ServerFieldType password_type,
                            const PasswordForm& submitted_form,
                            FieldTypeMap* field_types) {
  DCHECK(password_type == autofill::NEW_PASSWORD ||
         password_type == autofill::PROBABLY_NEW_PASSWORD ||
         password_type == autofill::NOT_NEW_PASSWORD)
      << password_type;
  DCHECK(!submitted_form.new_password_element.empty());

  (*field_types)[submitted_form.password_element] = autofill::PASSWORD;
  (*field_types)[submitted_form.new_password_element] = password_type;
}

// Sets the autofill type of the password field stored in |submitted_form| to
// |password_type| in |field_types| map.
void SetFieldLabelsOnSave(const autofill::ServerFieldType password_type,
                          const PasswordForm& submitted_form,
                          FieldTypeMap* field_types) {
  DCHECK(password_type == autofill::PASSWORD ||
         password_type == autofill::PROBABLY_ACCOUNT_CREATION_PASSWORD ||
         password_type == autofill::ACCOUNT_CREATION_PASSWORD ||
         password_type == autofill::NOT_ACCOUNT_CREATION_PASSWORD)
      << password_type;

  if (!submitted_form.new_password_element.empty()) {
    (*field_types)[submitted_form.new_password_element] = password_type;
  } else {
    DCHECK(!submitted_form.password_element.empty());
    (*field_types)[submitted_form.password_element] = password_type;
  }
}

// Label username and password fields with autofill types in |form_structure|
// based on |field_types|. The function also adds the types to
// |available_field_types|.
void LabelFields(const FieldTypeMap& field_types,
                 FormStructure* form_structure,
                 autofill::ServerFieldTypeSet* available_field_types) {
  for (size_t i = 0; i < form_structure->field_count(); ++i) {
    autofill::AutofillField* field = form_structure->field(i);

    autofill::ServerFieldType type = autofill::UNKNOWN_TYPE;
    if (!field->name.empty()) {
      auto iter = field_types.find(field->name);
      if (iter != field_types.end()) {
        type = iter->second;
        available_field_types->insert(type);
      }
    }

    autofill::ServerFieldTypeSet types;
    types.insert(type);
    field->set_possible_types(types);
  }
}

}  // namespace

VoteUploaderImpl::VoteUploaderImpl(PasswordManagerClient* client)
    : client_(client) {}

VoteUploaderImpl::~VoteUploaderImpl() = default;

void VoteUploaderImpl::SendVoteOnCredentialsReuse(
    const PasswordForm& observed,
    const PasswordForm* submitted_form,
    bool has_generated_password,
    bool username_edited,
    PasswordForm* pending) {
  // Ignore |pending| if its FormData has no fields. This is to weed out those
  // credentials that were saved before FormData was added to PasswordForm.
  // Even without this check, these FormStructure's won't be uploaded, but it
  // makes it hard to see if we are encountering unexpected errors.
  if (pending->form_data.fields.empty())
    return;

  FormStructure pending_structure(pending->form_data);
  FormStructure observed_structure(observed.form_data);

  if (pending_structure.form_signature() !=
      observed_structure.form_signature()) {
    // Only upload if this is the first time the password has been used.
    // Otherwise the credentials have been used on the same field before so
    // they aren't from an account creation form.
    // Also bypass uploading if the username was edited. Offering generation
    // in cases where we currently save the wrong username isn't great.
    // TODO(gcasto): Determine if generation should be offered in this case.
    if (pending->times_used == 1 && !username_edited) {
      if (UploadPasswordVote(
              *pending, submitted_form, autofill::ACCOUNT_CREATION_PASSWORD,
              observed_structure.FormSignatureAsStr(), has_generated_password,
              observed.IsPossibleChangePasswordForm())) {
        pending->generation_upload_status = PasswordForm::POSITIVE_SIGNAL_SENT;
      }
    }
  } else if (pending->generation_upload_status ==
             PasswordForm::POSITIVE_SIGNAL_SENT) {
    // A signal was sent that this was an account creation form, but the
    // credential is now being used on the same form again. This cancels out
    // the previous vote.
    if (UploadPasswordVote(*pending, submitted_form,
                           autofill::NOT_ACCOUNT_CREATION_PASSWORD,
                           std::string(), has_generated_password,
                           observed.IsPossibleChangePasswordForm())) {
      pending->generation_upload_status = PasswordForm::NEGATIVE_SIGNAL_SENT;
    }
  } else if (generation_popup_was_shown_) {
    // Even if there is no autofill vote to be sent, send the vote about the
    // usage of the generation popup.
    UploadPasswordVote(*pending, submitted_form, autofill::UNKNOWN_TYPE,
                       std::string(), has_generated_password,
                       observed.IsPossibleChangePasswordForm());
  }
}

bool VoteUploaderImpl::UploadPasswordVote(
    const PasswordForm& form_to_upload,
    const PasswordForm* submitted_form,
    autofill::ServerFieldType field_type,
    const std::string& login_form_signature,
    bool has_generated_password,
    bool observed_is_change_password_form) {
  // Abort if there is nothing to report for autofil or password generation.
  if (field_type == autofill::UNKNOWN_TYPE && !generation_popup_was_shown_)
    return false;

  autofill::AutofillManager* autofill_manager =
      client_->GetAutofillManagerForMainFrame();
  if (!autofill_manager || !autofill_manager->download_manager())
    return false;

  // If this is an update, a vote about the observed form is sent. If the user
  // re-uses credentials, a vote about the saved form is sent. If the user saves
  // credentials, the observed and pending forms are the same.
  FormStructure form_structure(form_to_upload.form_data);
  if (!autofill_manager->ShouldUploadForm(form_structure) ||
      !form_structure.ShouldBeCrowdsourced()) {
    UMA_HISTOGRAM_BOOLEAN("PasswordGeneration.UploadStarted", false);
    return false;
  }

  autofill::ServerFieldTypeSet available_field_types;
  if (field_type == autofill::USERNAME) {
    // Username correction vote.
    FieldTypeMap field_types;
    field_types[form_to_upload.username_element] = autofill::USERNAME;
    field_types[form_to_upload.password_element] =
        autofill::ACCOUNT_CREATION_PASSWORD;
    LabelFields(field_types, &form_structure, &available_field_types);
  } else {
    if (field_type != autofill::UNKNOWN_TYPE) {
      // A map from field names to field types.
      FieldTypeMap field_types;
      DCHECK(submitted_form);
      if (field_type == autofill::NEW_PASSWORD ||
          field_type == autofill::PROBABLY_NEW_PASSWORD ||
          field_type == autofill::NOT_NEW_PASSWORD) {
        // Update.
        if (submitted_form->new_password_element.empty())
          return false;
        SetFieldLabelsOnUpdate(field_type, *submitted_form, &field_types);
      } else {
        // Saving.
        SetFieldLabelsOnSave(field_type, *submitted_form, &field_types);
      }
      field_types[submitted_form->confirmation_password_element] =
          autofill::CONFIRMATION_PASSWORD;
      LabelFields(field_types, &form_structure, &available_field_types);
    }
    if (field_type != autofill::ACCOUNT_CREATION_PASSWORD) {
      if (generation_popup_was_shown_) {
        AddGeneratedVote(has_generated_password,
                         observed_is_change_password_form, &form_structure);
      }
      if (form_classifier_outcome_ != kNoOutcome)
        AddFormClassifierVote(&form_structure);
    }
  }

  // Force uploading. These events are relatively rare but important.
  form_structure.set_upload_required(UPLOAD_REQUIRED);

  if (password_manager_util::IsLoggingActive(client_)) {
    BrowserSavePasswordProgressLogger logger(client_->GetLogManager());
    logger.LogFormStructure(Logger::STRING_FORM_VOTES, form_structure);
  }

  const bool success = autofill_manager->download_manager()->StartUploadRequest(
      form_structure, false /* was_autofilled */, available_field_types,
      login_form_signature, true /* observed_submission */);

  UMA_HISTOGRAM_BOOLEAN("PasswordGeneration.UploadStarted", success);
  return success;
}

void VoteUploaderImpl::SendVotesOnSave(const PasswordForm& observed_form,
                                       const PasswordForm& submitted_form,
                                       bool has_generated_password,
                                       bool username_edited,
                                       PasswordForm* pending_credentials) {
  if (observed_form.IsPossibleChangePasswordFormWithoutUsername())
    return;

  // Send votes for sign-in form.
  autofill::FormData& form_data = pending_credentials->form_data;
  if (form_data.fields.size() == 2 &&
      form_data.fields[0].form_control_type == "text" &&
      form_data.fields[1].form_control_type == "password") {
    // |form_data| is received from the renderer and does not contain field
    // values. Fill username field value with username to allow AutofillManager
    // to detect username autofill type.
    form_data.fields[0].value = pending_credentials->username_value;
    SendSignInVote(form_data);
    return;
  }

  // Upload credentials the first time they are saved. This data is used
  // by password generation to help determine account creation sites.
  // Credentials that have been previously used (e.g., PSL matches) are checked
  // to see if they are valid account creation forms.
  if (pending_credentials->times_used == 0) {
    UploadPasswordVote(*pending_credentials, &submitted_form,
                       submitted_form.does_look_like_signup_form
                           ? autofill::PROBABLY_ACCOUNT_CREATION_PASSWORD
                           : autofill::PASSWORD,
                       std::string(), has_generated_password,
                       observed_form.IsPossibleChangePasswordForm());
    if (corrected_username_) {
      UploadPasswordVote(
          *corrected_username_, &submitted_form, autofill::USERNAME,
          FormStructure(observed_form.form_data).FormSignatureAsStr(),
          has_generated_password, observed_form.IsPossibleChangePasswordForm());
    }
  } else {
    SendVoteOnCredentialsReuse(observed_form, &submitted_form,
                               has_generated_password, username_edited,
                               pending_credentials);
  }
}

void VoteUploaderImpl::SaveGenerationFieldDetectedByClassifier(
    const base::string16& generation_field) {
  form_classifier_outcome_ =
      generation_field.empty() ? kNoGenerationElement : kFoundGenerationElement;
  generation_element_detected_by_classifier_ = generation_field;
}

void VoteUploaderImpl::NotifyGenerationPopupShown(
    const base::string16& generation_element,
    bool is_manual) {
  generation_popup_was_shown_ = true;
  generation_element_ = generation_element;
  is_manual_generation_ = is_manual;
}

void VoteUploaderImpl::NotifyUsernameCorrected(
    autofill::PasswordForm corrected_username) {
  DCHECK(!corrected_username_);
  corrected_username_ = std::move(corrected_username);
}

void VoteUploaderImpl::NotifyPasswordChangedByUser(bool changed) {
  generated_password_changed_ = changed;
}

void VoteUploaderImpl::AddGeneratedVote(bool has_generated_password,
                                        bool observed_is_change_password_form,
                                        FormStructure* form_structure) {
  DCHECK(form_structure);
  DCHECK(generation_popup_was_shown_);

  if (generation_element_.empty())
    return;

  autofill::AutofillUploadContents::Field::PasswordGenerationType type =
      autofill::AutofillUploadContents::Field::NO_GENERATION;
  if (has_generated_password) {
    if (is_manual_generation_) {
      type = observed_is_change_password_form
                 ? autofill::AutofillUploadContents::Field::
                       MANUALLY_TRIGGERED_GENERATION_ON_CHANGE_PASSWORD_FORM
                 : autofill::AutofillUploadContents::Field::
                       MANUALLY_TRIGGERED_GENERATION_ON_SIGN_UP_FORM;
    } else {
      type =
          observed_is_change_password_form
              ? autofill::AutofillUploadContents::Field::
                    AUTOMATICALLY_TRIGGERED_GENERATION_ON_CHANGE_PASSWORD_FORM
              : autofill::AutofillUploadContents::Field::
                    AUTOMATICALLY_TRIGGERED_GENERATION_ON_SIGN_UP_FORM;
    }
  } else
    type = autofill::AutofillUploadContents::Field::IGNORED_GENERATION_POPUP;

  for (size_t i = 0; i < form_structure->field_count(); ++i) {
    autofill::AutofillField* field = form_structure->field(i);
    if (field->name == generation_element_) {
      field->set_generation_type(type);
      field->set_generated_password_changed(generated_password_changed_);
      break;
    }
  }
}

void VoteUploaderImpl::AddFormClassifierVote(FormStructure* form_structure) {
  DCHECK(form_structure);
  DCHECK_NE(form_classifier_outcome_, kNoOutcome);

  for (size_t i = 0; i < form_structure->field_count(); ++i) {
    autofill::AutofillField* field = form_structure->field(i);
    if (form_classifier_outcome_ == kFoundGenerationElement &&
        field->name == generation_element_detected_by_classifier_) {
      field->set_form_classifier_outcome(
          autofill::AutofillUploadContents::Field::GENERATION_ELEMENT);
    } else {
      field->set_form_classifier_outcome(
          autofill::AutofillUploadContents::Field::NON_GENERATION_ELEMENT);
    }
  }
}

void VoteUploaderImpl::SendSignInVote(const autofill::FormData& form_data) {
  autofill::AutofillManager* autofill_manager =
      client_->GetAutofillManagerForMainFrame();
  if (!autofill_manager)
    return;
  auto form_structure = base::MakeUnique<FormStructure>(form_data);
  form_structure->set_is_signin_upload(true);
  DCHECK(form_structure->ShouldBeCrowdsourced());
  DCHECK_EQ(2u, form_structure->field_count());
  form_structure->field(1)->set_possible_types({autofill::PASSWORD});
  autofill_manager->StartUploadProcess(std::move(form_structure),
                                       base::TimeTicks::Now(), true);
}

}  // namespace password_manager
