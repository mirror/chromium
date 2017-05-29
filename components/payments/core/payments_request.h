// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PAYMENTS_CORE_PAYMENTS_REQUEST_H_
#define COMPONENTS_PAYMENTS_CORE_PAYMENTS_REQUEST_H_

#include <memory>

#include "base/values.h"

namespace payments {

class PaymentsClientDelegate;

enum PaymentsRpcResult {
  // Empty result. Used for initializing variables and should generally
  // not be returned nor passed as arguments unless explicitly allowed by
  // the API.
  NONE,

  // Request succeeded.
  SUCCESS,

  // Request failed; try again.
  TRY_AGAIN_FAILURE,

  // Request failed; don't try again.
  PERMANENT_FAILURE,

  // Unable to connect to Payments servers. Prompt user to check internet
  // connection.
  NETWORK_ERROR,
};

enum UnmaskCardReason {
  // The card is being unmasked for PaymentRequest.
  UNMASK_FOR_PAYMENT_REQUEST,

  // The card is being unmasked for Autofill.
  UNMASK_FOR_AUTOFILL,
};

// Interface for the various Payments request types.
class PaymentsRequest {
 public:
  virtual ~PaymentsRequest() {}

  // Returns the URL path for this type of request.
  virtual std::string GetRequestUrlPath() = 0;

  // Returns the content type that should be used in the HTTP request.
  virtual std::string GetRequestContentType() = 0;

  // Returns the content that should be provided in the HTTP request.
  virtual std::string GetRequestContent() = 0;

  // Parses the required elements of the HTTP response.
  virtual void ParseResponse(
      std::unique_ptr<base::DictionaryValue> response) = 0;

  // Returns true if all of the required elements were successfully retrieved by
  // a call to ParseResponse.
  virtual bool IsResponseComplete() = 0;

  // Invokes the appropriate callback in the delegate based on what type of
  // request this is.
  virtual void RespondToDelegate(PaymentsClientDelegate* delegate,
                                 PaymentsRpcResult result) = 0;
};

}  // namespace payments

#endif  // COMPONENTS_PAYMENTS_CORE_PAYMENTS_REQUEST_H_
