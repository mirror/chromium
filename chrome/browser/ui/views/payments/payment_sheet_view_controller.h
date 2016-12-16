// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_PAYMENTS_PAYMENT_SHEET_VIEW_CONTROLLER_H_
#define CHROME_BROWSER_UI_VIEWS_PAYMENTS_PAYMENT_SHEET_VIEW_CONTROLLER_H_

#include "base/macros.h"
#include "chrome/browser/ui/views/payments/payment_request_sheet_controller.h"
#include "ui/views/controls/button/button.h"

namespace payments {

class PaymentRequestImpl;
class PaymentRequestDialog;

// The PaymentRequestSheetController subtype for the Payment Sheet screen of the
// Payment Request dialog.
class PaymentSheetViewController : public PaymentRequestSheetController,
                                   public views::ButtonListener {
 public:
  // Does not take ownership of the arguments, which should outlive this object.
  PaymentSheetViewController(PaymentRequestImpl* impl,
                             PaymentRequestDialog* dialog);
  ~PaymentSheetViewController() override;

  // PaymentRequestSheetController:
  std::unique_ptr<views::View> CreateView() override;

 private:
  // views::ButtonListener:
  void ButtonPressed(views::Button* sender, const ui::Event& event) override;

  DISALLOW_COPY_AND_ASSIGN(PaymentSheetViewController);
};

}  // namespace payments

#endif  // CHROME_BROWSER_UI_VIEWS_PAYMENTS_PAYMENT_SHEET_VIEW_CONTROLLER_H_
