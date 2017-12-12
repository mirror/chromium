// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PAYMENTS_CONTENT_PAYMENT_REQUEST_DISPLAY_MANAGER_H_
#define COMPONENTS_PAYMENTS_CONTENT_PAYMENT_REQUEST_DISPLAY_MANAGER_H_

#include "base/macros.h"
#include "components/keyed_service/core/keyed_service.h"

namespace payments {

class ContentPaymentRequestDelegate;
class PaymentRequest;

class PaymentRequestDisplayManager : public KeyedService {
 public:
  PaymentRequestDisplayManager();
  ~PaymentRequestDisplayManager() override;

  bool CanShow() const;
  void Show(ContentPaymentRequestDelegate* delegate, PaymentRequest* request);
  void HideIfNecessary(PaymentRequest* request);

 private:
  PaymentRequest* showing_;

  DISALLOW_COPY_AND_ASSIGN(PaymentRequestDisplayManager);
};

}  // namespace payments

#endif  // COMPONENTS_PAYMENTS_CONTENT_PAYMENT_REQUEST_DISPLAY_MANAGER_H_
