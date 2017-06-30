// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_VOTE_UPLOADER_IMPL_H_
#define COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_VOTE_UPLOADER_IMPL_H_

#include "components/password_manager/core/browser/vote_uploader.h"

#include "base/macros.h"
#include "base/optional.h"
#include "base/strings/string16.h"
#include "components/autofill/core/common/password_form.h"

namespace autofill {
struct FormData;
class FormStructure;
}  // namespace autofill

namespace password_manager {

class PasswordManagerClient;

// Encapsulates handling of votes about form fields to be sent to the server.
// This is bound to a single password form.
class VoteUploaderImpl : public VoteUploader {
 public:
  explicit VoteUploaderImpl(PasswordManagerClient* client);
  ~VoteUploaderImpl() override;

  // VoteUploader
  void SendVoteOnCredentialsReuse(const autofill::PasswordForm& observed,
                                  const autofill::PasswordForm* submitted_form,
                                  bool has_generated_password,
                                  bool username_edited,
                                  autofill::PasswordForm* pending) override;
  bool UploadPasswordVote(const autofill::PasswordForm& form_to_upload,
                          const autofill::PasswordForm* submitted_form,
                          autofill::ServerFieldType field_type,
                          const std::string& login_form_signature,
                          bool has_generated_password,
                          bool observed_is_change_password_form) override;
  void SendVotesOnSave(const autofill::PasswordForm& observed_form,
                       const autofill::PasswordForm& submitted_form,
                       bool has_generated_password,
                       bool username_edited,
                       autofill::PasswordForm* pending_credentials) override;
  void SaveGenerationFieldDetectedByClassifier(
      const base::string16& generation_field) override;
  void NotifyGenerationPopupShown(const base::string16& generation_element,
                                  bool is_manual) override;
  void NotifyUsernameCorrected(
      autofill::PasswordForm corrected_username) override;
  void NotifyPasswordChangedByUser(bool changed) override;

 private:
  // The outcome of the form classifier.
  enum FormClassifierOutcome {
    kNoOutcome,
    kNoGenerationElement,
    kFoundGenerationElement
  };

  // Adds a vote on password generation usage, in particluar based on whether
  // the pending credential |has_generated_password|, to |form_structure|.
  // |observed_is_change_password_form| tells whether the observed form is
  // likely a change password form.
  void AddGeneratedVote(bool has_generated_password,
                        bool observed_is_change_password_form,
                        autofill::FormStructure* form_structure);

  // Adds a vote from HTML parsing based form classifier to |form_structure|.
  void AddFormClassifierVote(autofill::FormStructure* form_structure);

  // Send a vote for sign-in forms with autofill types for a username field.
  void SendSignInVote(const autofill::FormData& form_data);

  // Saved credentials can contain multiple usernames: username_value, which is
  // the one Chrome assumes should be filled, and alternative usernames, which
  // come from other fields in the web form. If Chrome notices that the user
  // has corrected the filled username to some of the alternatives, the stored
  // credential with the corrected name of the field storing the alternative is
  // noted in |corrected_username_| and used to create a correction vote.
  base::Optional<autofill::PasswordForm> corrected_username_;

  // The outcome of HTML parsing based form classifier.
  FormClassifierOutcome form_classifier_outcome_ = kNoOutcome;

  // The name of the detected generation element if |form_classifier_outcome_|
  // is kFoundGenerationElement.
  base::string16 generation_element_detected_by_classifier_;

  // The client which implements embedder-specific PasswordManager operations.
  PasswordManagerClient* const client_;

  // Whether generation popup was shown at least once.
  bool generation_popup_was_shown_ = false;

  // Whether password generation was manually triggered.
  bool is_manual_generation_ = false;

  // The name of the password field used for generation, if any.
  base::string16 generation_element_;

  // Whether the reported generate password was adjusted by the user.
  bool generated_password_changed_ = false;

  DISALLOW_COPY_AND_ASSIGN(VoteUploaderImpl);
};

}  // namespace password_manager

#endif  // COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_VOTE_UPLOADER_IMPL_H_
