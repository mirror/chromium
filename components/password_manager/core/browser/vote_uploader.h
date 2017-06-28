// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_VOTE_UPLOADER_H_
#define COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_VOTE_UPLOADER_H_

#include <string>

#include "base/strings/string16.h"
#include "components/autofill/core/browser/field_types.h"

namespace autofill {
struct PasswordForm;
}

namespace password_manager {

// Encapsulates handling of votes about form fields to be sent to the server.
// This is bound to a single password form.
class VoteUploader {
 public:
  VoteUploader() {}
  virtual ~VoteUploader() {}

  // If |*pending| corresponds to an account creation form, send a vote to the
  // Autofill server to vote for the correct username field, and also to
  // support triggering password generation on that form in the future. This
  // function will update generation_upload_status of |pending| if an upload is
  // performed. |observed| is the form currently observed on the page.
  // |has_generated_password| tells whether the submitted password was
  // generated. |username_edited| captures whether the user chose an alternative
  // username from other possible usernames.
  virtual void SendVoteOnCredentialsReuse(
      const autofill::PasswordForm& observed,
      const autofill::PasswordForm* submitted_form,
      bool has_generated_password,
      bool username_edited,
      autofill::PasswordForm* pending) = 0;

  // Tries to set all votes (e.g. autofill field types, generation vote) to a
  // |FormStructure| and upload it to the server. Returns true on success.
  // |form_to_upload| is the form for which the vote is being sent,
  // |submitted_form| points to the form which was submitted, if any,
  // |field_type| is the type of the field the vote is speaking about, and
  // |login_form_signature| is the signature of the associated login form if the
  // vote is for the account creation form. |has_generated_password| is true iff
  // |form_to_upload| has a generated password.
  // |observed_is_change_password_form| tells whether the observed form is
  // likely a change password form.
  virtual bool UploadPasswordVote(const autofill::PasswordForm& form_to_upload,
                                  const autofill::PasswordForm* submitted_form,
                                  autofill::ServerFieldType field_type,
                                  const std::string& login_form_signature,
                                  bool has_generated_password,
                                  bool observed_is_change_password_form) = 0;

  // Send appropriate votes based on which credential is currently being saved.
  // |observed_form| represents what is on the page, |submitted_form| what was
  // submitted and |pending_credentials| what is ready to be saved to
  // PasswordStore. |has_generated_password| captures, whether the password in
  // |pending_credentials| was generated. |username_edited| captures whether the
  // user chose an alternative username from other possible usernames.
  virtual void SendVotesOnSave(const autofill::PasswordForm& observed_form,
                               const autofill::PasswordForm& submitted_form,
                               bool has_generated_password,
                               bool username_edited,
                               autofill::PasswordForm* pending_credentials) = 0;

  // Saves the HTML-based classification of what is the generation field.
  virtual void SaveGenerationFieldDetectedByClassifier(
      const base::string16& generation_field) = 0;

  // Call this when the generation pop-up was shown for |generation_element|.
  // Set |is_manual| according to whether the user triggered the generation
  // (true) or whether it happened automatically (false).
  virtual void NotifyGenerationPopupShown(
      const base::string16& generation_element,
      bool is_manual) = 0;

  virtual void NotifyUsernameCorrected(
      autofill::PasswordForm corrected_username) = 0;

  // Call this when the user changes an already generated password (passing
  // |true|) or if the generated password is seen for the first time or
  // cancelled (passing |false|).
  virtual void NotifyPasswordChangedByUser(bool changed) = 0;
};

}  // namespace password_manager

#endif  // COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_VOTE_UPLOADER_H_
