// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/payments/ios_payment_instrument_launcher.h"

#include <map>

#include "base/base64.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/memory/ptr_util.h"
#include "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "components/payments/core/payment_instrument.h"
#include "ios/web/public/navigation_item.h"
#include "ios/web/public/navigation_manager.h"
#include "ios/web/public/payments/payment_request.h"
#include "ios/web/public/ssl_status.h"
#include "ios/web/public/web_state/web_state.h"
#include "net/base/mac/url_conversions.mm"
#include "net/base/url_util.h"
#include "net/cert/x509_certificate.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

// Parameters sent to a payment app.
static const char kMethodNames[] = "methodNames";
static const char kMethodData[] = "methodData";
static const char kMerchantName[] = "merchantName";
static const char kTopLevelOrigin[] = "topLevelOrigin";
static const char kTopLevelCertificateChain[] = "topLevelCertificateChain";
static const char kCertificate[] = "cert";
static const char kTotal[] = "total";
static const char kModifiers[] = "modifiers";

// Parameters received from a payment app.
static const char kSuccess[] = "success";
static const char kMethodName[] = "methodName";
static const char kDetails[] = "details";

}  // namespace

namespace payments {

IOSPaymentInstrumentLauncher::IOSPaymentInstrumentLauncher()
    : delegate_(nullptr), payment_request_id_("") {}

IOSPaymentInstrumentLauncher::~IOSPaymentInstrumentLauncher() {}

void IOSPaymentInstrumentLauncher::LaunchIOSPaymentInstrument(
    payments::PaymentRequest* payment_request,
    web::WebState* active_web_state,
    std::string& universal_link,
    payments::PaymentInstrument::Delegate* delegate) {
  DCHECK(delegate);

  // Only one request can be handled at a time.
  if (delegate_ || !payment_request_id_.empty()) {
    delegate_ = delegate;
    CompleteLaunchRequest("", "");
    return;
  }

  delegate_ = delegate;

  std::unique_ptr<base::DictionaryValue> params_to_payment_app =
      base::MakeUnique<base::DictionaryValue>();

  // TODO(crbug.com/748556): Filter the following list to only show method names
  // that we know the payment app supports. For now, sending all the requested
  // method names i.e., 'basic-card' and 'https://alice-pay.com" to the payment
  // app works as the payment app provider can then decide what to do with that
  // information, but this is not ideal nor is this consistent with Android
  // implementation.
  std::unique_ptr<base::ListValue> methodNames =
      base::MakeUnique<base::ListValue>();
  for (std::map<std::string, std::set<std::string>>::const_iterator it =
           payment_request->stringified_method_data().begin();
       it != payment_request->stringified_method_data().end(); ++it) {
    methodNames->GetList().emplace_back(it->first);
  }
  params_to_payment_app->SetList(kMethodNames, std::move(methodNames));

  params_to_payment_app->SetDictionary(
      kMethodData,
      SerializeMethodData(payment_request->stringified_method_data()));

  params_to_payment_app->SetString(
      kMerchantName, base::UTF16ToASCII(active_web_state->GetTitle()));

  params_to_payment_app->SetStringWithoutPathExpansion(
      kTopLevelOrigin, active_web_state->GetLastCommittedURL().host());

  params_to_payment_app->SetList(
      kTopLevelCertificateChain,
      SerializeCertificateChain(
          active_web_state->GetNavigationManager()->GetVisibleItem()));

  params_to_payment_app->SetDictionary(
      kTotal, payment_request->web_payment_request()
                  .details.total.amount.ToDictionaryValue());

  params_to_payment_app->SetList(
      kModifiers,
      SerializeModifiers(payment_request->web_payment_request().details));

  // JSON stringify the object so that it can be encoded in base-64.
  std::string stringified_parameters;
  base::JSONWriter::Write(*params_to_payment_app, &stringified_parameters);
  std::string base_64_params;
  base::Base64Encode(stringified_parameters, &base_64_params);

  payment_request_id_ =
      payment_request->web_payment_request().payment_request_id;

  GURL universal_link_gurl = GURL(universal_link);
  universal_link_gurl = net::AppendQueryParameter(
      universal_link_gurl, web::kPaymentRequestIDExternal, payment_request_id_);
  universal_link_gurl = net::AppendQueryParameter(
      universal_link_gurl, web::kPaymentRequestDataExternal, base_64_params);
  NSURL* url = net::NSURLWithGURL(universal_link_gurl);
  [[UIApplication sharedApplication] openURL:url
      options:@{
        UIApplicationOpenURLOptionUniversalLinksOnly : @YES
      }
      completionHandler:^(BOOL success) {
        if (!success) {
          CompleteLaunchRequest("", "");
        }
      }];
}

void IOSPaymentInstrumentLauncher::ReceiveResponseFromIOSPaymentInstrument(
    const std::string& base_64_response) {
  DCHECK(delegate_);

  std::string stringified_parameters;
  base::Base64Decode(base_64_response, &stringified_parameters);

  std::unique_ptr<base::Value> value =
      base::JSONReader::Read(stringified_parameters);
  if (!value) {
    CompleteLaunchRequest("", "");
    return;
  }

  std::unique_ptr<base::DictionaryValue> dict =
      base::DictionaryValue::From(std::move(value));
  if (!dict) {
    CompleteLaunchRequest("", "");
    return;
  }

  int success;
  if (!dict->GetInteger(kSuccess, &success) || success == 0) {
    CompleteLaunchRequest("", "");
    return;
  }

  std::string method_name;
  if (!dict->GetString(kMethodName, &method_name) || method_name.empty()) {
    CompleteLaunchRequest("", "");
    return;
  }

  std::string stringified_details;
  if (!dict->GetString(kDetails, &stringified_details) ||
      stringified_details.empty()) {
    CompleteLaunchRequest("", "");
    return;
  }

  CompleteLaunchRequest(method_name, stringified_details);
}

std::unique_ptr<base::DictionaryValue>
IOSPaymentInstrumentLauncher::SerializeMethodData(
    const std::map<std::string, std::set<std::string>>&
        stringified_method_data) {
  std::unique_ptr<base::DictionaryValue> method_data =
      base::MakeUnique<base::DictionaryValue>();

  std::unique_ptr<base::ListValue> data_list;
  std::unique_ptr<base::Value> data;
  for (std::map<std::string, std::set<std::string>>::const_iterator map_it =
           stringified_method_data.begin();
       map_it != stringified_method_data.end(); ++map_it) {
    data_list = base::MakeUnique<base::ListValue>();
    for (std::set<std::string>::iterator data_it = map_it->second.begin();
         data_it != map_it->second.end(); ++data_it) {
      data = base::JSONReader().ReadToValue(*data_it);
      // We insert the stringified data, not the JSON object and only if the
      // corresponding JSON object is valid.
      if (data)
        data_list->GetList().emplace_back(*data_it);
    }

    method_data->SetListWithoutPathExpansion(map_it->first,
                                             std::move(data_list));
  }

  return method_data;
}

std::unique_ptr<base::ListValue>
IOSPaymentInstrumentLauncher::SerializeCertificateChain(
    web::NavigationItem* item) {
  std::unique_ptr<base::ListValue> certChainList =
      base::MakeUnique<base::ListValue>();

  if (!item)
    return certChainList;

  scoped_refptr<net::X509Certificate> cert = item->GetSSL().certificate;
  std::vector<std::vector<const char>> cert_chain;
  net::X509Certificate::OSCertHandles cert_handles =
      cert->GetIntermediateCertificates();
  if (cert_handles.empty() || cert_handles[0] != cert->os_cert_handle())
    cert_handles.insert(cert_handles.begin(), cert->os_cert_handle());

  cert_chain.reserve(cert_handles.size());
  for (auto* handle : cert_handles) {
    std::string cert_bytes;
    net::X509Certificate::GetDEREncoded(handle, &cert_bytes);
    cert_chain.push_back(
        std::vector<const char>(cert_bytes.begin(), cert_bytes.end()));
  }

  std::unique_ptr<base::DictionaryValue> certChainDict;
  std::unique_ptr<base::ListValue> byteArray;
  for (std::vector<const char> cert_string : cert_chain) {
    byteArray = base::MakeUnique<base::ListValue>();
    certChainDict = base::MakeUnique<base::DictionaryValue>();
    for (const char byte : cert_string) {
      byteArray->GetList().emplace_back(byte);
    }
    certChainDict->Set(kCertificate, std::move(byteArray));
    certChainList->GetList().push_back(*certChainDict);
  }

  return certChainList;
}

std::unique_ptr<base::ListValue>
IOSPaymentInstrumentLauncher::SerializeModifiers(web::PaymentDetails details) {
  std::unique_ptr<base::ListValue> modifiers =
      base::MakeUnique<base::ListValue>();
  size_t numModifiers = details.modifiers.size();
  for (size_t i = 0; i < numModifiers; ++i) {
    const std::unique_ptr<base::DictionaryValue> modifier =
        details.modifiers[i].ToDictionaryValue();
    modifiers->GetList().push_back(*modifier.get());
  }

  return modifiers;
}

void IOSPaymentInstrumentLauncher::CompleteLaunchRequest(
    const std::string& method_name,
    const std::string& details) {
  if (!method_name.empty() && !details.empty())
    delegate_->OnInstrumentDetailsReady(method_name, details);
  else
    delegate_->OnInstrumentDetailsError();
  delegate_ = nullptr;
  payment_request_id_ = "";
}

}  // namespace payments
