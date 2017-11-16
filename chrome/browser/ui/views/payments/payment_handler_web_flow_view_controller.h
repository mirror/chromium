// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_PAYMENTS_PAYMENT_HANDLER_WEB_FLOW_VIEW_CONTROLLER_H_
#define CHROME_BROWSER_UI_VIEWS_PAYMENTS_PAYMENT_HANDLER_WEB_FLOW_VIEW_CONTROLLER_H_

#include "chrome/browser/ui/views/payments/payment_request_sheet_controller.h"
#include "components/payments/core/payment_request_base_delegate.h"
#include "content/public/browser/web_contents_observer.h"

class Profile;

namespace payments {

class PaymentRequestDialogView;
class PaymentRequestSpec;
class PaymentRequestState;

class PaymentHandlerWebFlowViewController
    : public PaymentRequestSheetController,
      public content::WebContentsObserver {
 public:
  PaymentHandlerWebFlowViewController(
      PaymentRequestSpec* spec,
      PaymentRequestState* state,
      PaymentRequestDialogView* dialog,
      Profile* profile,
      std::string payment_method,
      GURL flow_override_url,
      GURL success_url,
      GURL failure_url,
      std::string payment_method_data,
      PaymentRequestBaseDelegate::InstrumentDetailsReadyCallback callback);
  ~PaymentHandlerWebFlowViewController() override;

  base::string16 GetSheetTitle() override;
  void FillContentView(views::View* content_view) override;

 private:
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;
  Profile* profile_;
  std::string payment_method_;
  GURL flow_override_url_;
  GURL success_url_;
  GURL failure_url_;
  std::string payment_method_data_;
  PaymentRequestBaseDelegate::InstrumentDetailsReadyCallback callback_;
};

}  // namespace payments

#endif  // CHROME_BROWSER_UI_VIEWS_PAYMENTS_PAYMENT_HANDLER_WEB_FLOW_VIEW_CONTROLLER_H_
