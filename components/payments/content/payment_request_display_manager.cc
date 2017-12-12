// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/payments/content/payment_request_display_manager.h"

#include "base/logging.h"
#include "components/payments/content/content_payment_request_delegate.h"

namespace payments {

class PaymentRequest;

PaymentRequestDisplayManager::PaymentRequestDisplayManager() {}
PaymentRequestDisplayManager::~PaymentRequestDisplayManager() {}

bool PaymentRequestDisplayManager::CanShow() const {
  return !showing_;
}

void PaymentRequestDisplayManager::Show(ContentPaymentRequestDelegate* delegate,
                                        PaymentRequest* request) {
  DCHECK(request);
  DCHECK(CanShow());
  if (showing_ == request) {
    NOTREACHED() << "Trying to show the same Payment Request twice";
    return;
  }

  showing_ = request;
  delegate->ShowDialog(request);
}

void PaymentRequestDisplayManager::HideIfNecessary(PaymentRequest* request) {
  if (showing_ == request)
    showing_ = nullptr;
}

}  // namespace payments
