// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/payments/payment_handler_web_flow_view_controller.h"

#include "base/base64.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/navigation_handle.h"
#include "ui/views/controls/webview/webview.h"
#include "ui/views/layout/fill_layout.h"

namespace payments {

PaymentHandlerWebFlowViewController::PaymentHandlerWebFlowViewController(
    PaymentRequestSpec* spec,
    PaymentRequestState* state,
    PaymentRequestDialogView* dialog,
    Profile* profile,
    std::string payment_method,
    GURL flow_override_url,
    GURL success_url,
    GURL failure_url,
    std::string payment_method_data,
    PaymentRequestBaseDelegate::InstrumentDetailsReadyCallback callback)
    : PaymentRequestSheetController(spec, state, dialog),
      profile_(profile),
      payment_method_(payment_method),
      flow_override_url_(flow_override_url),
      success_url_(success_url),
      failure_url_(failure_url),
      payment_method_data_(payment_method_data),
      callback_(callback) {}

PaymentHandlerWebFlowViewController::~PaymentHandlerWebFlowViewController() {}

base::string16 PaymentHandlerWebFlowViewController::GetSheetTitle() {
  return base::string16();
}

void PaymentHandlerWebFlowViewController::FillContentView(
    views::View* content_view) {
  content_view->SetLayoutManager(new views::FillLayout);
  std::unique_ptr<views::WebView> web_view =
      base::MakeUnique<views::WebView>(profile_);
  Observe(web_view->GetWebContents());

  GURL::Replacements replacements;
  std::string query =
      "success=" + success_url_.spec() + "&failure=" + failure_url_.spec();
  if (!payment_method_data_.empty()) {
    std::string encoded_data;
    base::Base64Encode(payment_method_data_, &encoded_data);
    query += "&data=" + encoded_data;
  }

  replacements.SetQuery(query.c_str(), url::Component(0, query.length()));
  GURL flow_url = flow_override_url_.is_empty()
                      ? GURL(payment_method_).ReplaceComponents(replacements)
                      : flow_override_url_.ReplaceComponents(replacements);

  web_view->LoadInitialURL(flow_url);
  web_view->SetPreferredSize(gfx::Size(100, 300));
  content_view->AddChildView(web_view.release());
}

void PaymentHandlerWebFlowViewController::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  if (navigation_handle->GetURL().GetOrigin() !=
      flow_override_url_.GetOrigin()) {
    NOTREACHED();  // TODO(anthonyvd): abort PR because we navigated away from
                   // the handler's domain
  }

  if (navigation_handle->GetURL().path() == success_url_.path()) {
    std::string query = navigation_handle->GetURL().query();

    std::string decoded_details;
    const std::string kDetailsKey = "details=";
    size_t details_start = query.find(kDetailsKey);
    if (details_start != std::string::npos) {
      details_start += kDetailsKey.length();
      size_t details_end = query.find("&", details_start);
      details_end =
          details_end == std::string::npos ? query.length() : details_end;

      std::string details_value =
          query.substr(details_start, details_end - details_start);
      if (!base::Base64Decode(details_value, &decoded_details)) {
        NOTREACHED();  // TODO(anthonyvd): abort PR because there's a details
                       // field but it's invalid
      }
    }

    callback_.Run(payment_method_, decoded_details);
  }
}

}  // namespace payments
