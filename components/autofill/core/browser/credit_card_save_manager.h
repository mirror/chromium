// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_AUTOFILL_CORE_BROWSER_CREDIT_CARD_SAVE_MANAGER_H_
#define COMPONENTS_AUTOFILL_CORE_BROWSER_CREDIT_CARD_SAVE_MANAGER_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/callback_forward.h"
#include "base/compiler_specific.h"
#include "base/containers/circular_deque.h"
#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/string16.h"
#include "base/time/time.h"
#include "build/build_config.h"
#include "components/autofill/core/browser/autocomplete_history_manager.h"
#include "components/autofill/core/browser/autofill_client.h"
#include "components/autofill/core/browser/autofill_download_manager.h"
#include "components/autofill/core/browser/autofill_driver.h"
#include "components/autofill/core/browser/autofill_handler.h"
#include "components/autofill/core/browser/autofill_metrics.h"
#include "components/autofill/core/browser/card_unmask_delegate.h"
#include "components/autofill/core/browser/form_structure.h"
#include "components/autofill/core/browser/payments/full_card_request.h"
#include "components/autofill/core/browser/payments/payments_client.h"
#include "components/autofill/core/browser/personal_data_manager.h"
#include "components/autofill/core/common/form_data.h"

namespace autofill {

// Manages logic for determining whether credit card save is available and
// actioning the actual save, including upload credit card save to Google
// Payments.  Owned by AutofillManager.
class CreditCardSaveManager {
 public:
  // The parameters should outlive the CreditCardSaveManager.
  CreditCardSaveManager(AutofillClient* client,
                        payments::PaymentsClient* payments_client,
                        const std::string& app_locale,
                        PersonalDataManager* personal_data_manager);
  virtual ~CreditCardSaveManager();

  // Checks the result of the GetDetails RPC to Google Payments and determines
  // if and which offer-to-save credit card bubble should be displayed to the
  // user.
  void CheckUploadGetDetailsResult(
      AutofillClient::PaymentsRpcResult result,
      const base::string16& context_token,
      std::unique_ptr<base::DictionaryValue> legal_message);

  // Checks the result of the SaveCard RPC to Google Payments after the user
  // attempted to upload their card.
  void CheckUploadSaveCardResult(AutofillClient::PaymentsRpcResult result,
                                 const std::string& server_id);

  // Offers local credit card save to the user.
  void OfferCardLocalSave(const CreditCard& card);

  // Begins the process to offer upload credit card save to the user if the
  // imported card passes all requirements and Payments approves.
  void AttemptToOfferCardUploadSave(const FormStructure& submitted_form,
                                    const CreditCard& card);

  // Returns true if all the conditions for enabling the upload of credit card
  // are satisfied.
  virtual bool IsCreditCardUploadEnabled();

  // For testing.
  void set_app_locale(std::string app_locale) { app_locale_ = app_locale; }

 private:
  // Examines |card| and the stored profiles and if a candidate set of profiles
  // is found that matches the client-side validation rules, assigns the values
  // to |upload_request.profiles| and returns 0. If no valid set can be found,
  // returns the failure reasons. Appends any experiments that were triggered to
  // |upload_request.active_experiments|. The return value is a bitmask of
  // |AutofillMetrics::CardUploadDecisionMetric|.
  int SetProfilesForCreditCardUpload(
      const CreditCard& card,
      payments::PaymentsClient::UploadRequestDetails* upload_request) const;

  // Sets |user_did_accept_upload_prompt_| and calls UploadCard if the risk data
  // is available.
  void OnUserDidAcceptUpload();

  // Saves risk data in |uploading_risk_data_| and calls UploadCard if the user
  // has accepted the prompt.
  void OnDidGetUploadRiskData(const std::string& risk_data);

  // Returns metric relevant to the CVC field based on values in
  // |found_cvc_field_|, |found_value_in_cvc_field_| and
  // |found_cvc_value_in_non_cvc_field_|.
  AutofillMetrics::CardUploadDecisionMetric GetCVCCardUploadDecisionMetric()
      const;

  // Logs the card upload decisions in UKM and UMA.
  // |upload_decision_metrics| is a bitmask of
  // |AutofillMetrics::CardUploadDecisionMetric|.
  void LogCardUploadDecisions(int upload_decision_metrics);

  AutofillClient* const client_;

  // Handles Payments service requests.
  // Owned by AutofillManager.
  payments::PaymentsClient* payments_client_;

  std::string app_locale_;

  // The personal data manager, used to save and load personal data to/from the
  // web database.  This is overridden by the AutofillManagerTest.
  // Weak reference.
  // May be NULL.  NULL indicates OTR.
  PersonalDataManager* personal_data_manager_;

  // Collected information about a pending upload request.
  payments::PaymentsClient::UploadRequestDetails upload_request_;
  bool user_did_accept_upload_prompt_;

  // |should_cvc_be_requested_| is |true| if we should request CVC from the user
  // in the card upload dialog.
  bool should_cvc_be_requested_;
  // |found_cvc_field_| is |true| if there exists a field that is determined to
  // be a CVC field via heuristics.
  bool found_cvc_field_;
  // |found_value_in_cvc_field_| is |true| if a field that is determined to
  // be a CVC field via heuristics has non-empty |value|.
  // |value| may or may not be a valid CVC.
  bool found_value_in_cvc_field_;
  // |found_cvc_value_in_non_cvc_field_| is |true| if a field that is not
  // determined to be a CVC field via heuristics has a valid CVC |value|.
  bool found_cvc_value_in_non_cvc_field_;

  GURL pending_upload_request_url_;

  base::WeakPtrFactory<CreditCardSaveManager> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(CreditCardSaveManager);
};

}  // namespace autofill

#endif  // COMPONENTS_AUTOFILL_CORE_BROWSER_CREDIT_CARD_SAVE_MANAGER_H_
