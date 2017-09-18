// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/payments/content/service_worker_payment_instrument.h"

#include "base/strings/utf_string_conversions.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/payment_app_provider.h"
#include "ui/gfx/image/image_skia.h"

namespace payments {

// Service worker payment app provides icon through bitmap, so set 0 as invalid
// resource Id.
ServiceWorkerPaymentInstrument::ServiceWorkerPaymentInstrument(
    content::BrowserContext* context,
    const GURL& top_level_origin,
    const GURL& frame_origin,
    const mojom::PaymentDetails* details,
    const std::vector<mojom::PaymentMethodDataPtr>* method_data,
    std::unique_ptr<content::StoredPaymentApp> stored_payment_app_info)
    : PaymentInstrument(0, PaymentInstrument::Type::SERVICE_WORKER_APP),
      browser_context_(context),
      top_level_origin_(top_level_origin),
      frame_origin_(frame_origin),
      details_(details),
      method_data_(method_data),
      stored_payment_app_info_(std::move(stored_payment_app_info)),
      weak_ptr_factory_(this) {
  DCHECK(browser_context_);
  DCHECK(top_level_origin_.is_valid());
  DCHECK(frame_origin_.is_valid());
  DCHECK(details_);
  DCHECK(method_data_);

  if (stored_payment_app_info_->icon) {
    icon_image_ =
        gfx::ImageSkia::CreateFrom1xBitmap(*(stored_payment_app_info_->icon))
            .DeepCopy();
  }
}

ServiceWorkerPaymentInstrument::~ServiceWorkerPaymentInstrument() {}

void ServiceWorkerPaymentInstrument::InvokePaymentApp(Delegate* delegate) {
  mojom::PaymentRequestEventDataPtr event_data =
      mojom::PaymentRequestEventData::New();

  event_data->top_level_origin = top_level_origin_;
  event_data->payment_request_origin = frame_origin_;

  if (details_->id.has_value())
    event_data->payment_request_id = details_->id.value();

  event_data->total = mojom::PaymentCurrencyAmount::New();
  *(event_data->total) = *(details_->total->amount);

  std::unordered_set<std::string> supported_methods;
  supported_methods.insert(stored_payment_app_info_->enabled_methods.begin(),
                           stored_payment_app_info_->enabled_methods.end());
  for (const auto& modifier : details_->modifiers) {
    std::vector<std::string>::const_iterator it =
        modifier->method_data->supported_methods.begin();
    for (; it != modifier->method_data->supported_methods.end(); it++) {
      if (supported_methods.find(*it) != supported_methods.end())
        break;
    }
    if (it == modifier->method_data->supported_methods.end())
      continue;

    mojom::PaymentDetailsModifierPtr modifier_copy =
        mojom::PaymentDetailsModifier::New();

    modifier_copy->total = mojom::PaymentItem::New();
    modifier_copy->total->label = modifier->total->label;

    modifier_copy->total->amount = mojom::PaymentCurrencyAmount::New();
    *(modifier_copy->total->amount) = *(modifier->total->amount);

    modifier_copy->method_data = mojom::PaymentMethodData::New();
    modifier_copy->method_data->supported_methods =
        modifier->method_data->supported_methods;
    modifier_copy->method_data->stringified_data =
        modifier->method_data->stringified_data;

    event_data->modifiers.emplace_back(std::move(modifier_copy));
  }

  for (const auto& data : *method_data_) {
    std::vector<std::string>::const_iterator it =
        data->supported_methods.begin();
    for (; it != data->supported_methods.end(); it++) {
      if (supported_methods.find(*it) != supported_methods.end())
        break;
    }
    if (it == data->supported_methods.end())
      continue;

    mojom::PaymentMethodDataPtr method_data_copy =
        mojom::PaymentMethodData::New();
    method_data_copy->supported_methods = data->supported_methods;
    method_data_copy->stringified_data = data->stringified_data;
    event_data->method_data.push_back(std::move(method_data_copy));
  }

  delegate_ = delegate;

  content::PaymentAppProvider::GetInstance()->InvokePaymentApp(
      browser_context_, stored_payment_app_info_->registration_id,
      std::move(event_data),
      base::BindOnce(&ServiceWorkerPaymentInstrument::OnPaymentAppInvoked,
                     weak_ptr_factory_.GetWeakPtr()));
}

void ServiceWorkerPaymentInstrument::OnPaymentAppInvoked(
    mojom::PaymentHandlerResponsePtr response) {
  if (delegate_ != nullptr) {
    delegate_->OnInstrumentDetailsReady(response->method_name,
                                        response->stringified_details);
    delegate_ = nullptr;
  }
}

bool ServiceWorkerPaymentInstrument::IsCompleteForPayment() const {
  return true;
}

bool ServiceWorkerPaymentInstrument::IsExactlyMatchingMerchantRequest() const {
  return true;
}

base::string16 ServiceWorkerPaymentInstrument::GetMissingInfoLabel() const {
  NOTREACHED();
  return base::string16();
}

bool ServiceWorkerPaymentInstrument::IsValidForCanMakePayment() const {
  NOTIMPLEMENTED();
  return true;
}

void ServiceWorkerPaymentInstrument::RecordUse() {
  NOTIMPLEMENTED();
}

base::string16 ServiceWorkerPaymentInstrument::GetLabel() const {
  return base::UTF8ToUTF16(stored_payment_app_info_->name);
}

base::string16 ServiceWorkerPaymentInstrument::GetSublabel() const {
  return base::UTF8ToUTF16(stored_payment_app_info_->scope.GetOrigin().spec());
}

bool ServiceWorkerPaymentInstrument::IsValidForModifier(
    const std::vector<std::string>& method,
    const std::vector<std::string>& supported_networks,
    const std::set<autofill::CreditCard::CardType>& supported_types,
    bool supported_types_specified) const {
  NOTIMPLEMENTED();
  return false;
}

const gfx::ImageSkia* ServiceWorkerPaymentInstrument::icon_image_skia() const {
  return icon_image_.get();
}

}  // namespace payments
