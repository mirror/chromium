// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/payments/ios_payment_app_finder.h"

#include "base/bind.h"
#include "base/json/json_reader.h"
#include "base/memory/ptr_util.h"
#include "base/values.h"
#include "components/payments/core/payment_manifest_downloader.h"
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
static const char kRelatedApplicationsID[] = "id";

}  // namespace

namespace payments {

IOSPaymentAppFinder::IOSPaymentAppFinder(
    const Callback& callback,
    net::URLRequestContextGetter* context_getter)
    : callback_(callback),
      context_getter_(context_getter),
      weak_factory_(this) {
  DCHECK(!callback_.is_null());
}

IOSPaymentAppFinder::~IOSPaymentAppFinder() {}

bool IOSPaymentAppFinder::PaymentAppRespondsToScheme(
    const std::string& url_scheme) {
  // TODO(crbug.com/60266): Implement this method to call canOpenURL from
  // UIAppliaction on the passed in scheme.
  return true;
}

void IOSPaymentAppFinder::DownloadManifests(const GURL& method) {
  // TODO(crbug.com/60266): Set scheme to the corresponding url scheme of the
  // given url payment method identifier. A dictionary of payment method
  // identifiers to scheme names is currently maintained in the IOSPaymentApp
  // class which is currently in review. That dictionary will also likely be
  // moved to this class when both CLs land.
  std::string scheme = "";
  if (!PaymentAppRespondsToScheme(scheme))
    return;

  downloader_ = base::MakeUnique<PaymentManifestDownloader>(context_getter_);
  downloader_->DownloadPaymentMethodManifest(
      method, base::BindOnce(&IOSPaymentAppFinder::OnPaymentManifestDownload,
                             weak_factory_.GetWeakPtr()));
}

void IOSPaymentAppFinder::OnPaymentManifestDownload(
    const std::string& content) {
  GURL web_app_manifest_url;
  bool success =
      ParsePaymentManifestForWebAppManifestURL(content, &web_app_manifest_url);

  if (success) {
    downloader_->DownloadWebAppManifest(
        GURL(web_app_manifest_url),
        base::BindOnce(&IOSPaymentAppFinder::OnWebAppManifestDownload,
                       weak_factory_.GetWeakPtr()));
  }
}

bool IOSPaymentAppFinder::ParsePaymentManifestForWebAppManifestURL(
    const std::string& input,
    GURL* web_app_manifest_url) {
  DCHECK(web_app_manifest_url->is_empty());

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

    *web_app_manifest_url = url;
    break;
  }

  // If we weren't able to find a valid web app manifest URL then we return
  // false.
  return !web_app_manifest_url->is_empty();
}

void IOSPaymentAppFinder::OnWebAppManifestDownload(const std::string& content) {
  std::string app_name;
  std::string app_icon;
  GURL universal_link;
  bool success = ParseWebAppManifestForPaymentAppDetails(
      content, &app_name, &app_icon, &universal_link);

  if (success)
    callback_.Run(app_name, app_icon, universal_link);
}

bool IOSPaymentAppFinder::ParseWebAppManifestForPaymentAppDetails(
    const std::string& input,
    std::string* app_name,
    std::string* app_icon,
    GURL* universal_link) {
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

  if (!dict->GetString(kShortName, app_name) || app_name->empty()) {
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

    std::string icon_url;
    if (!icon->GetString(kIconsSource, &icon_url) || icon_url.empty())
      continue;

    // We select the first valid icon in the list.
    *app_icon = icon_url;
    break;
  }

  if (app_icon->empty())
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

    if (!related_application->HasKey(kRelatedApplicationsID))
      continue;

    std::string link;
    if (!related_application->GetString(kRelatedApplicationsID, &link) ||
        link.empty())
      continue;

    GURL url(link);
    if (!url.is_valid() || !url.SchemeIs(url::kHttpsScheme))
      continue;

    *universal_link = url;
    break;
  }

  return !universal_link->is_empty();
}

}  // namespace payments
