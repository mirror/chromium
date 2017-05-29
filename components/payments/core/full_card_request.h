// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PAYMENTS_CORE_FULL_CARD_REQUEST_H_
#define COMPONENTS_PAYMENTS_CORE_FULL_CARD_REQUEST_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/string16.h"
#include "base/time/time.h"
#include "components/payments/core/card_unmask_delegate.h"
#include "components/payments/core/payments_client.h"

namespace autofill {
class CreditCard;
class PersonalDataManager;
class RiskDataLoader;
}

namespace payments {

// Retrieves the full card details, including the pan and the cvc.
class FullCardRequest : public CardUnmaskDelegate {
 public:
  // The interface for receiving the full card details.
  class ResultDelegate {
   public:
    virtual ~ResultDelegate() = default;
    virtual void OnFullCardRequestSucceeded(const autofill::CreditCard& card,
                                            const base::string16& cvc) = 0;
    virtual void OnFullCardRequestFailed() = 0;
  };

  // The delegate responsible for displaying the unmask prompt UI.
  class UIDelegate {
   public:
    virtual ~UIDelegate() = default;
    virtual void ShowUnmaskPrompt(
        const autofill::CreditCard& card,
        UnmaskCardReason reason,
        base::WeakPtr<CardUnmaskDelegate> delegate) = 0;
    virtual void OnUnmaskVerificationResult(PaymentsRpcResult result) = 0;
  };

  // The parameters should outlive the FullCardRequest.
  FullCardRequest(autofill::RiskDataLoader* risk_data_loader,
                  PaymentsClient* payments_client,
                  autofill::PersonalDataManager* personal_data_manager);
  ~FullCardRequest();

  // Retrieves the pan and cvc for |card| and invokes
  // Delegate::OnFullCardRequestSucceeded() or
  // Delegate::OnFullCardRequestFailed(). Only one request should be active at a
  // time.
  //
  // If the card is local, has a non-empty GUID, and the user has updated its
  // expiration date, then this function will write the new information to
  // autofill table on disk.
  void GetFullCard(const autofill::CreditCard& card,
                   UnmaskCardReason reason,
                   base::WeakPtr<ResultDelegate> result_delegate,
                   base::WeakPtr<UIDelegate> ui_delegate);

  // Returns true if there's a pending request to get the full card.
  bool IsGettingFullCard() const;

  // Called by the payments client when a card has been unmasked.
  void OnDidGetRealPan(PaymentsRpcResult result, const std::string& real_pan);

 private:
  // CardUnmaskDelegate:
  void OnUnmaskResponse(const UnmaskResponse& response) override;
  void OnUnmaskPromptClosed() override;

  // Called by autofill client when the risk data has been loaded.
  void OnDidGetUnmaskRiskData(const std::string& risk_data);

  // Resets the state of the request.
  void Reset();

  // Used to fetch risk data for this request.
  autofill::RiskDataLoader* const risk_data_loader_;

  // Responsible for unmasking a masked server card.
  PaymentsClient* const payments_client_;

  // Responsible for updating the server card on disk after it's been unmasked.
  autofill::PersonalDataManager* const personal_data_manager_;

  // Receiver of the full PAN and CVC.
  base::WeakPtr<ResultDelegate> result_delegate_;

  // Delegate responsible for displaying the unmask prompt UI.
  base::WeakPtr<UIDelegate> ui_delegate_;

  // The pending request to get a card's full PAN and CVC.
  std::unique_ptr<PaymentsClient::UnmaskRequestDetails> request_;

  // Whether the card unmask request should be sent to the payment server.
  bool should_unmask_card_;

  // The timestamp when the full PAN was requested from a server. For
  // histograms.
  base::Time real_pan_request_timestamp_;

  // Enables destroying FullCardRequest while CVC prompt is showing or a server
  // communication is pending.
  base::WeakPtrFactory<FullCardRequest> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(FullCardRequest);
};

}  // namespace payments

#endif  // COMPONENTS_PAYMENTS_CORE_FULL_CARD_REQUEST_H_
