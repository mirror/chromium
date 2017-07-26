// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/payments/ios_payment_instrument_finder.h"

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

#include "base/bind.h"
#include "base/json/json_reader.h"
#include "base/memory/ptr_util.h"
#include "base/strings/sys_string_conversions.h"
#include "base/threading/sequenced_worker_pool.h"
#include "base/values.h"
#include "components/image_fetcher/ios/ios_image_data_fetcher_wrapper.h"
#include "components/payments/core/payment_manifest_downloader.h"
#include "ios/chrome/browser/payments/ios_payment_instrument.h"
#include "ios/web/public/web_thread.h"
#include "net/url_request/url_request_context_getter.h"

namespace {

// The following constants are defined in one of the following two documents:
// https://w3c.github.io/payment-method-manifest/
// https://w3c.github.io/manifest/
static const char kDefaultApplications[] = "default_applications";
static const char kShortName[] = "short_name";
static const char kIcons[] = "icons";
static const char kIconsSource[] = "src";
static const char kRelatedApplications[] = "related_applications";
static const char kRelatedApplicationsUrl[] = "url";

}  // namespace

namespace payments {

IOSPaymentInstrumentFinder::IOSPaymentInstrumentFinder(
    net::URLRequestContextGetter* context_getter,
    id<PaymentRequestUIDelegate> payment_request_ui_delegate)
    : context_getter_(context_getter),
      payment_request_ui_delegate_(payment_request_ui_delegate),
      weak_factory_(this) {}

IOSPaymentInstrumentFinder::~IOSPaymentInstrumentFinder() {}

void IOSPaymentInstrumentFinder::GetValidURLPaymentMethods(
    const std::vector<std::string>& url_payment_method_identifiers,
    std::vector<std::string>* out_url_payment_method_identifiers) {
  DCHECK(out_url_payment_method_identifiers->empty());

  for (const std::string& method : url_payment_method_identifiers) {
    // Ensure that the payment method is recognized.
    const auto& enum_map = payments::GetMethodNameToSchemeName();
    const auto& entry = enum_map.find(method);
    if (entry == enum_map.end()) {
      continue;
    }

    // Ensure that there is an installed payment app on the user's device that
    // can handle the payment method.
    std::map<std::string, std::string> map =
        payments::GetMethodNameToSchemeName();
    std::string scheme = base::SysWideToUTF8(base::SysUTF8ToWide(map[method]));
    UIApplication* application = [UIApplication sharedApplication];
    NSURL* URL = [NSURL URLWithString:(base::SysUTF8ToNSString(scheme))];
    if (![application canOpenURL:URL]) {
      continue;
    }

    // If we've reached this point there must be an installed payment app that
    // can handle the recognized payment method.
    out_url_payment_method_identifiers->push_back(method);
  }
}

void IOSPaymentInstrumentFinder::GetIOSPaymentInstrumentsForMethods(
    const std::vector<GURL>& methods,
    const IOSPaymentInstrumentsFoundCallback& callback) {
  // If |callback_| is not null, there's already an active search for iOS
  // payment instruments, which shouldn't happen.
  DCHECK(callback_.is_null());

  // The function should immediately return if there were 0 methods supplied.
  if (methods.size() == 0)
    callback.Run(std::move(instruments_found_));

  callback_ = callback;
  num_payment_methods_remaining_ = methods.size();

  instruments_found_.clear();
  instruments_found_.reserve(methods.size());

  if (!downloader_)
    downloader_ = base::MakeUnique<PaymentManifestDownloader>(context_getter_);

  for (const GURL& method : methods) {
    downloader_->DownloadPaymentMethodManifest(
        method,
        base::BindOnce(&IOSPaymentInstrumentFinder::OnPaymentManifestDownloaded,
                       weak_factory_.GetWeakPtr(), method));
  }
}

void IOSPaymentInstrumentFinder::OnPaymentManifestDownloaded(
    const GURL& method,
    const std::string& content) {
  // If |content| is empty then the download failed.
  if (content.empty()) {
    DecreaseNumPaymentMethodsRemaining();
    return;
  }

  GURL web_app_manifest_url;
  if (ParsePaymentManifestForWebAppManifestURL(content,
                                               &web_app_manifest_url)) {
    downloader_->DownloadWebAppManifest(
        GURL(web_app_manifest_url),
        base::BindOnce(&IOSPaymentInstrumentFinder::OnWebAppManifestDownloaded,
                       weak_factory_.GetWeakPtr(), method));
  } else {
    DecreaseNumPaymentMethodsRemaining();
  }
}

bool IOSPaymentInstrumentFinder::ParsePaymentManifestForWebAppManifestURL(
    const std::string& input,
    GURL* out_web_app_manifest_url) {
  DCHECK(out_web_app_manifest_url->is_empty());

  std::unique_ptr<base::Value> value(base::JSONReader::Read(input));
  if (!value) {
    LOG(ERROR) << "Payment method manifest must be in JSON format.";
    return false;
  }

  std::unique_ptr<base::DictionaryValue> dict =
      base::DictionaryValue::From(std::move(value));
  if (!dict) {
    LOG(ERROR) << "Payment method manifest must be a JSON dictionary.";
    return false;
  }

  if (!dict->HasKey(kDefaultApplications))
    return false;

  base::ListValue* list = nullptr;
  if (!dict->GetList(kDefaultApplications, &list)) {
    LOG(ERROR) << "\"" << kDefaultApplications << "\" must be a list.";
    return false;
  }

  size_t apps_number = list->GetSize();
  if (apps_number == 0) {
    LOG(ERROR) << "\"" << kDefaultApplications << "\" must contain at least 1"
               << " entry.";
    return false;
  }

  std::string item;
  for (size_t i = 0; i < apps_number; ++i) {
    if (!list->GetString(i, &item) || item.empty())
      continue;

    GURL url(item);
    if (!url.is_valid() || !url.SchemeIs(url::kHttpsScheme))
      continue;

    *out_web_app_manifest_url = url;
    break;
  }

  // If we weren't able to find a valid web app manifest URL then we return
  // false.
  return !out_web_app_manifest_url->is_empty();
}

void IOSPaymentInstrumentFinder::OnWebAppManifestDownloaded(
    const GURL& method,
    const std::string& content) {
  // If |content| is empty then the download failed.
  if (content.empty()) {
    DecreaseNumPaymentMethodsRemaining();
    return;
  }

  std::string app_name;
  GURL app_icon_url;
  GURL universal_link;
  if (ParseWebAppManifestForPaymentAppDetails(content, &app_name, &app_icon_url,
                                              &universal_link)) {
    CreateIOSPaymentInstrument(method, app_name, app_icon_url, universal_link);
  } else {
    DecreaseNumPaymentMethodsRemaining();
  }
}

bool IOSPaymentInstrumentFinder::ParseWebAppManifestForPaymentAppDetails(
    const std::string& input,
    std::string* out_app_name,
    GURL* out_app_icon_url,
    GURL* out_universal_link) {
  std::unique_ptr<base::Value> value(base::JSONReader::Read(input));
  if (!value) {
    LOG(ERROR) << "Web app manifest must be in JSON format.";
    return false;
  }

  std::unique_ptr<base::DictionaryValue> dict =
      base::DictionaryValue::From(std::move(value));
  if (!dict) {
    LOG(ERROR) << "Web app manifest must be a JSON dictionary.";
    return false;
  }

  if (!dict->HasKey(kShortName))
    return false;

  if (!dict->GetString(kShortName, out_app_name) || out_app_name->empty()) {
    LOG(ERROR) << "\"" << kShortName << "\" must be a non-empty ASCII string.";
    return false;
  }

  base::ListValue* icons = nullptr;
  if (!dict->GetList(kIcons, &icons)) {
    LOG(ERROR) << "\"" << kIcons << "\" must be a list.";
    return false;
  }

  size_t icons_size = icons->GetSize();
  if (icons_size == 0) {
    LOG(ERROR) << "\"" << kIcons << "\" must contain at least 1 entry.";
    return false;
  }

  for (size_t i = 0; i < icons_size; ++i) {
    base::DictionaryValue* icon = nullptr;
    if (!icons->GetDictionary(i, &icon) || !icon)
      continue;

    std::string icon_string;
    if (!icon->GetString(kIconsSource, &icon_string) || icon_string.empty())
      continue;

    GURL icon_url(icon_string);
    if (!icon_url.is_valid() || !icon_url.SchemeIs(url::kHttpsScheme))
      continue;

    // We select the first valid icon in the list.
    *out_app_icon_url = icon_url;
    break;
  }

  if (out_app_icon_url->is_empty())
    return false;

  base::ListValue* apps = nullptr;
  if (!dict->GetList(kRelatedApplications, &apps)) {
    LOG(ERROR) << "\"" << kRelatedApplications << "\" must be a list.";
    return false;
  }

  size_t related_applications_size = apps->GetSize();
  if (related_applications_size == 0) {
    LOG(ERROR) << "\"" << kRelatedApplications << "\" must contain at least 1"
               << " entry.";
    return false;
  }

  for (size_t i = 0; i < related_applications_size; ++i) {
    base::DictionaryValue* related_application = nullptr;
    if (!apps->GetDictionary(i, &related_application) || !related_application)
      continue;

    std::string platform;
    if (!related_application->GetString("platform", &platform) ||
        platform != "itunes")
      continue;

    if (!related_application->HasKey(kRelatedApplicationsUrl))
      continue;

    std::string link;
    if (!related_application->GetString(kRelatedApplicationsUrl, &link) ||
        link.empty())
      continue;

    GURL url(link);
    if (!url.is_valid() || !url.SchemeIs(url::kHttpsScheme))
      continue;

    *out_universal_link = url;
    break;
  }

  return !out_universal_link->is_empty();
}

void IOSPaymentInstrumentFinder::CreateIOSPaymentInstrument(
    const GURL& method_name,
    std::string& app_name,
    GURL& app_icon_url,
    GURL& universal_link) {
  DCHECK(context_getter_);

  GURL local_method_name(method_name);
  std::string local_app_name(app_name);
  GURL local_universal_link(universal_link);

  if (!image_fetcher_) {
    image_fetcher_ =
        base::MakeUnique<image_fetcher::IOSImageDataFetcherWrapper>(
            context_getter_.get(), web::WebThread::GetBlockingPool());
  }

  image_fetcher::IOSImageDataFetcherCallback callback =
      ^(NSData* data, const image_fetcher::RequestMetadata& metadata) {
        if (data) {
          UIImage* icon =
              [UIImage imageWithData:data scale:[UIScreen mainScreen].scale];
          instruments_found_.push_back(base::MakeUnique<IOSPaymentInstrument>(
              local_method_name.spec(), local_universal_link.spec(),
              local_app_name, icon, payment_request_ui_delegate_));
        }
        DecreaseNumPaymentMethodsRemaining();
      };
  image_fetcher_->FetchImageDataWebpDecoded(app_icon_url, callback);
}

void IOSPaymentInstrumentFinder::DecreaseNumPaymentMethodsRemaining() {
  DCHECK(callback_);

  num_payment_methods_remaining_ -= 1;
  if (num_payment_methods_remaining_ == 0)
    callback_.Run(std::move(instruments_found_));
}

}  // namespace payments
