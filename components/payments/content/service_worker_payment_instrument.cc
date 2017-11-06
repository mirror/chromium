// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/payments/content/service_worker_payment_instrument.h"

#include "base/stl_util.h"
#include "base/strings/utf_string_conversions.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/payment_app_provider.h"
#include "third_party/WebKit/public/platform/modules/payments/payment_request.mojom.h"
#include "ui/gfx/image/image_skia.h"

namespace payments {

namespace {

template <typename T>
bool HaveSharedElements(std::vector<T> a, std::vector<T> b) {
  for (const auto& e : a) {
    for (const auto& f : b) {
      if (e == f)
        return true;
    }
  }
  return false;
}

std::vector<std::string> FromBasicCardNetworkToString(
    const std::vector<int32_t>& networks) {
  std::vector<std::string> network_strings;

  const std::vector<std::string> kBasicCardNetworks{
      "amex",       "diners", "discover", "jcb",
      "mastercard", "mir",    "unionpay", "visa"};

  for (const auto& network : networks) {
    switch ((mojom::BasicCardNetwork)network) {
      case mojom::BasicCardNetwork::AMEX:
        network_strings.emplace_back(kBasicCardNetworks[0]);
        break;
      case mojom::BasicCardNetwork::DINERS:
        network_strings.emplace_back(kBasicCardNetworks[1]);
        break;
      case mojom::BasicCardNetwork::DISCOVER:
        network_strings.emplace_back(kBasicCardNetworks[2]);
        break;
      case mojom::BasicCardNetwork::JCB:
        network_strings.emplace_back(kBasicCardNetworks[3]);
        break;
      case mojom::BasicCardNetwork::MASTERCARD:
        network_strings.emplace_back(kBasicCardNetworks[4]);
        break;
      case mojom::BasicCardNetwork::MIR:
        network_strings.emplace_back(kBasicCardNetworks[5]);
        break;
      case mojom::BasicCardNetwork::UNIONPAY:
        network_strings.emplace_back(kBasicCardNetworks[6]);
        break;
      case mojom::BasicCardNetwork::VISA:
        network_strings.emplace_back(kBasicCardNetworks[7]);
        break;
      default:
        LOG(ERROR) << "Unknown BasicCardNetwork value: " << network;
    }
  }

  return network_strings;
}

std::vector<int32_t> FromCardTypeToBasicCardTypes(
    const std::set<autofill::CreditCard::CardType>& types) {
  std::vector<int32_t> basiccard_types;

  for (const auto& type : types) {
    switch (type) {
      case autofill::CreditCard::CardType::CARD_TYPE_UNKNOWN:
        // Do nothing.
        break;
      case autofill::CreditCard::CardType::CARD_TYPE_CREDIT:
        basiccard_types.emplace_back(
            static_cast<int32_t>(mojom::BasicCardType::CREDIT));
        break;
      case autofill::CreditCard::CardType::CARD_TYPE_DEBIT:
        basiccard_types.emplace_back(
            static_cast<int32_t>(mojom::BasicCardType::DEBIT));
        break;
      case autofill::CreditCard::CardType::CARD_TYPE_PREPAID:
        basiccard_types.emplace_back(
            static_cast<int32_t>(mojom::BasicCardType::PREPAID));
        break;
      default:
        LOG(ERROR) << "Invalid card type value: " << type;
    }
  }

  return basiccard_types;
}

}  // namespace

// Service worker payment app provides icon through bitmap, so set 0 as invalid
// resource Id.
ServiceWorkerPaymentInstrument::ServiceWorkerPaymentInstrument(
    content::BrowserContext* context,
    const GURL& top_level_origin,
    const GURL& frame_origin,
    const PaymentRequestSpec* spec,
    std::unique_ptr<content::StoredPaymentApp> stored_payment_app_info)
    : PaymentInstrument(0, PaymentInstrument::Type::SERVICE_WORKER_APP),
      browser_context_(context),
      top_level_origin_(top_level_origin),
      frame_origin_(frame_origin),
      spec_(spec),
      stored_payment_app_info_(std::move(stored_payment_app_info)),
      delegate_(nullptr),
      weak_ptr_factory_(this) {
  DCHECK(browser_context_);
  DCHECK(top_level_origin_.is_valid());
  DCHECK(frame_origin_.is_valid());
  DCHECK(spec_);

  if (stored_payment_app_info_->icon) {
    icon_image_ =
        gfx::ImageSkia::CreateFrom1xBitmap(*(stored_payment_app_info_->icon))
            .DeepCopy();
  }
}

ServiceWorkerPaymentInstrument::~ServiceWorkerPaymentInstrument() {
  if (delegate_) {
    // If there's a payment handler in progress, abort it before destroying this
    // so that it can close its window. Since the PaymentRequest will be
    // destroyed, pass an empty callback to the payment handler.
    content::PaymentAppProvider::GetInstance()->AbortPayment(
        browser_context_, stored_payment_app_info_->registration_id,
        base::Bind([](bool) {}));
  }
}

void ServiceWorkerPaymentInstrument::InvokePaymentApp(Delegate* delegate) {
  delegate_ = delegate;

  content::PaymentAppProvider::GetInstance()->InvokePaymentApp(
      browser_context_, stored_payment_app_info_->registration_id,
      CreatePaymentRequestEventData(),
      base::BindOnce(&ServiceWorkerPaymentInstrument::OnPaymentAppInvoked,
                     weak_ptr_factory_.GetWeakPtr()));
}

mojom::PaymentRequestEventDataPtr
ServiceWorkerPaymentInstrument::CreatePaymentRequestEventData() {
  mojom::PaymentRequestEventDataPtr event_data =
      mojom::PaymentRequestEventData::New();

  event_data->top_level_origin = top_level_origin_;
  event_data->payment_request_origin = frame_origin_;

  if (spec_->details().id.has_value())
    event_data->payment_request_id = spec_->details().id.value();

  event_data->total = spec_->details().total->amount.Clone();

  std::unordered_set<std::string> supported_methods;
  supported_methods.insert(stored_payment_app_info_->enabled_methods.begin(),
                           stored_payment_app_info_->enabled_methods.end());
  for (const auto& modifier : spec_->details().modifiers) {
    std::vector<std::string>::const_iterator it =
        modifier->method_data->supported_methods.begin();
    for (; it != modifier->method_data->supported_methods.end(); it++) {
      if (supported_methods.find(*it) != supported_methods.end())
        break;
    }
    if (it == modifier->method_data->supported_methods.end())
      continue;

    event_data->modifiers.emplace_back(modifier.Clone());
  }

  for (const auto& data : spec_->method_data()) {
    std::vector<std::string>::const_iterator it =
        data->supported_methods.begin();
    for (; it != data->supported_methods.end(); it++) {
      if (supported_methods.find(*it) != supported_methods.end())
        break;
    }
    if (it == data->supported_methods.end())
      continue;

    event_data->method_data.push_back(data.Clone());
  }

  return event_data;
}

void ServiceWorkerPaymentInstrument::OnPaymentAppInvoked(
    mojom::PaymentHandlerResponsePtr response) {
  DCHECK(delegate_);

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
    bool supported_networks_specified,
    const std::vector<std::string>& supported_networks,
    bool supported_types_specified,
    const std::set<autofill::CreditCard::CardType>& supported_types) const {
  std::vector<std::string> supported_methods;
  for (const auto& modifier_supported_method : method) {
    if (base::ContainsValue(stored_payment_app_info_->enabled_methods,
                            modifier_supported_method)) {
      supported_methods.emplace_back(modifier_supported_method);
    }
  }

  if (supported_methods.empty()) {
    return false;
  }

  // Compare capabilities if 'basic-card' is the only supported payment method.
  if (supported_methods.size() == 1U && supported_methods[0] == "basic-card") {
    // Return true if both card networks and types are not specified.
    if (!supported_networks_specified && !supported_types_specified)
      return true;

    if (stored_payment_app_info_->capabilities.empty())
      return false;

    uint32_t i = 0;
    for (; i < stored_payment_app_info_->capabilities.size(); i++) {
      if (supported_networks_specified) {
        std::vector<std::string> app_supported_networks =
            FromBasicCardNetworkToString(
                stored_payment_app_info_->capabilities[i]
                    .supported_card_networks);
        if (!HaveSharedElements(app_supported_networks, supported_networks))
          continue;
      }

      if (supported_types_specified) {
        std::vector<int32_t> app_supported_types =
            FromCardTypeToBasicCardTypes(supported_types);
        if (!HaveSharedElements(app_supported_types,
                                stored_payment_app_info_->capabilities[i]
                                    .supported_card_types)) {
          continue;
        }
      }

      break;
    }

    if (i >= stored_payment_app_info_->capabilities.size())
      return false;
  }

  return true;
}

const gfx::ImageSkia* ServiceWorkerPaymentInstrument::icon_image_skia() const {
  return icon_image_.get();
}

}  // namespace payments
