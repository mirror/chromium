// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_PAYMENTS_CVC_UNMASK_VIEW_CONTROLLER_H_
#define CHROME_BROWSER_UI_VIEWS_PAYMENTS_CVC_UNMASK_VIEW_CONTROLLER_H_

#include <string>

#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/observer_list.h"
#include "base/strings/string16.h"
#include "chrome/browser/ui/autofill/autofill_dialog_models.h"
#include "chrome/browser/ui/views/payments/payment_request_sheet_controller.h"
#include "components/autofill/core/browser/risk_data_loader.h"
#include "components/payments/core/full_card_request.h"
#include "components/payments/core/payments_client.h"
#include "ui/views/controls/combobox/combobox_listener.h"
#include "ui/views/controls/textfield/textfield_controller.h"

namespace content {
class WebContents;
}

namespace views {
class Textfield;
}

namespace payments {

class PaymentRequestSpec;
class PaymentRequestState;
class PaymentRequestDialogView;

class CvcUnmaskViewController : public PaymentRequestSheetController,
                                public autofill::RiskDataLoader,
                                public PaymentsClientDelegate,
                                public FullCardRequest::UIDelegate,
                                public views::ComboboxListener,
                                public views::TextfieldController {
 public:
  CvcUnmaskViewController(
      PaymentRequestSpec* spec,
      PaymentRequestState* state,
      PaymentRequestDialogView* dialog,
      const autofill::CreditCard& credit_card,
      base::WeakPtr<FullCardRequest::ResultDelegate> result_delegate,
      content::WebContents* web_contents);
  ~CvcUnmaskViewController() override;

  // PaymentsClientDelegate:
  IdentityProvider* GetIdentityProvider() override;
  void OnDidGetRealPan(PaymentsRpcResult result,
                       const std::string& real_pan) override;
  void OnDidGetUploadDetails(
      PaymentsRpcResult result,
      const base::string16& context_token,
      std::unique_ptr<base::DictionaryValue> legal_message) override;
  void OnDidUploadCard(PaymentsRpcResult result,
                       const std::string& server_id) override;

  // autofill::RiskDataLoader:
  void LoadRiskData(
      const base::Callback<void(const std::string&)>& callback) override;

  // FullCardRequest::UIDelegate:
  void ShowUnmaskPrompt(const autofill::CreditCard& card,
                        UnmaskCardReason reason,
                        base::WeakPtr<CardUnmaskDelegate> delegate) override;
  void OnUnmaskVerificationResult(PaymentsRpcResult result) override;

 protected:
  // PaymentRequestSheetController:
  base::string16 GetSheetTitle() override;
  void FillContentView(views::View* content_view) override;
  std::unique_ptr<views::Button> CreatePrimaryButton() override;
  void ButtonPressed(views::Button* sender, const ui::Event& event) override;

 private:
  // Called when the user confirms their CVC. This will pass the value to the
  // active FullCardRequest.
  void CvcConfirmed();

  // Display a label with the text |error|
  void DisplayError(base::string16 error);

  // Updates the enabled state of the pay button
  void UpdatePayButtonState();

  bool GetSheetId(DialogViewID* sheet_id) override;
  views::View* GetFirstFocusedView() override;

  // views::TextfieldController:
  void ContentsChanged(views::Textfield* sender,
                       const base::string16& new_contents) override;

  // views::ComboboxListener:
  void OnPerformAction(views::Combobox* combobox) override;

  autofill::MonthComboboxModel month_combobox_model_;
  autofill::YearComboboxModel year_combobox_model_;
  views::Textfield* cvc_field_;  // owned by the view hierarchy, outlives this.
  autofill::CreditCard credit_card_;
  content::WebContents* web_contents_;
  // The identity provider, used for Payments integration.
  std::unique_ptr<IdentityProvider> identity_provider_;
  PaymentsClient payments_client_;
  FullCardRequest full_card_request_;
  base::WeakPtr<CardUnmaskDelegate> unmask_delegate_;

  base::WeakPtrFactory<CvcUnmaskViewController> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(CvcUnmaskViewController);
};

}  // namespace payments

#endif  // CHROME_BROWSER_UI_VIEWS_PAYMENTS_CVC_UNMASK_VIEW_CONTROLLER_H_
