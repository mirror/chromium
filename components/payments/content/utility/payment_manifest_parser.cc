// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/payments/content/utility/payment_manifest_parser.h"

#include <algorithm>
#include <utility>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "components/payments/content/origin_security_checker.h"
#include "components/payments/content/utility/fingerprint_parser.h"
#include "content/public/common/service_manager_connection.h"
#include "net/base/url_util.h"
#include "services/data_decoder/public/cpp/safe_json_parser.h"

namespace payments {
namespace {

const size_t kMaximumNumberOfItems = 100U;

const char* const kDefaultApplications = "default_applications";
const char* const kSupportedOrigins = "supported_origins";

// Parses the "default_applications": ["https://some/url"] from |dict| into
// |web_app_manifest_urls|. Returns 'false' for invalid data.
bool ParseDefaultApplications(base::DictionaryValue* dict,
                              const GURL& manifest_location,
                              std::vector<GURL>* web_app_manifest_urls) {
  DCHECK(dict);
  DCHECK(web_app_manifest_urls);

  base::ListValue* list = nullptr;
  if (!dict->GetList(kDefaultApplications, &list)) {
    LOG(ERROR) << "\"" << kDefaultApplications << "\" must be a list.";
    return false;
  }

  size_t apps_number = list->GetSize();
  if (apps_number > kMaximumNumberOfItems) {
    LOG(ERROR) << "\"" << kDefaultApplications << "\" must contain at most "
               << kMaximumNumberOfItems << " entries.";
    return false;
  }

  for (size_t i = 0; i < apps_number; ++i) {
    std::string item;
    if (!list->GetString(i, &item)) {
      LOG(ERROR) << "Each item in \"" << kDefaultApplications
                 << "\" should be a string, but the item in position " << i
                 << " is not a string.";
      web_app_manifest_urls->clear();
      return false;
    }

    GURL url;
    std::string error_message_suffix;
    if (!OriginSecurityChecker::IsValidWebPaymentsUrl(
            item, manifest_location, &url, &error_message_suffix)) {
      LOG(ERROR) << "Entry \"" << item << "\" in \"" << kDefaultApplications
                 << "\" " << error_message_suffix;
      web_app_manifest_urls->clear();
      return false;
    }
    DCHECK(url.is_valid());

    web_app_manifest_urls->push_back(url);
  }

  return true;
}

// Parses the "supported_origins": "*" (or ["https://some.origin"]) from |dict|
// into |supported_origins| and |all_origins_supported|. Returns 'false' for
// invalid data.
bool ParseSupportedOrigins(base::DictionaryValue* dict,
                           std::vector<url::Origin>* supported_origins,
                           bool* all_origins_supported) {
  DCHECK(dict);
  DCHECK(supported_origins);
  DCHECK(all_origins_supported);

  *all_origins_supported = false;

  {
    std::string item;
    if (dict->GetString(kSupportedOrigins, &item)) {
      if (item != "*") {
        LOG(ERROR) << "\"" << item << "\" is not a valid value for \""
                   << kSupportedOrigins
                   << "\". Must be either \"*\" or a list of RFC6454 origins.";
        return false;
      }

      *all_origins_supported = true;
      return true;
    }
  }

  base::ListValue* list = nullptr;
  if (!dict->GetList(kSupportedOrigins, &list)) {
    LOG(ERROR) << "\"" << kSupportedOrigins
               << "\" must be either \"*\" or a list of origins.";
    return false;
  }

  size_t supported_origins_number = list->GetSize();
  const size_t kMaximumNumberOfSupportedOrigins = 100000;
  if (supported_origins_number > kMaximumNumberOfSupportedOrigins) {
    LOG(ERROR) << "\"" << kSupportedOrigins << "\" must contain at most "
               << kMaximumNumberOfSupportedOrigins << " entires.";
    return false;
  }

  for (size_t i = 0; i < supported_origins_number; ++i) {
    std::string item;
    if (!list->GetString(i, &item)) {
      LOG(ERROR) << "Each item in \"" << kSupportedOrigins
                 << "\" should be a string, but the item in position " << i
                 << " is not a string.";
      supported_origins->clear();
      return false;
    }

    GURL url;
    std::string error_message_suffix;
    if (!OriginSecurityChecker::IsValidWebPaymentsUrl(item, GURL(), &url,
                                                      &error_message_suffix)) {
      LOG(ERROR) << "Entry \"" << item << "\" in \"" << kSupportedOrigins
                 << "\" " << error_message_suffix;
      supported_origins->clear();
      return false;
    }
    DCHECK(url.is_valid());

    if (url.path() != "/" || url.has_query() || url.has_ref()) {
      LOG(ERROR) << "Entry \"" << item << "\" in \"" << kSupportedOrigins
                 << "\" is not a valid RFC6454 origin.";
      supported_origins->clear();
      return false;
    }

    supported_origins->push_back(url::Origin::Create(url));
  }

  return true;
}

}  // namespace

PaymentManifestParser::PaymentManifestParser() : weak_factory_(this) {}

PaymentManifestParser::~PaymentManifestParser() = default;

void PaymentManifestParser::ParsePaymentMethodManifest(
    const std::string& content,
    const GURL& manifest_location,
    PaymentMethodCallback success_callback,
    JsonParseErrorCallback parse_error_callback,
    InvalidDataCallback invalid_data_callback) {
  DCHECK_GT(10U, parse_payment_callback_counter_);
  parse_payment_callback_counter_++;

  data_decoder::SafeJsonParser::Parse(
      content::ServiceManagerConnection::GetForProcess()->GetConnector(),
      content,
      base::Bind(&PaymentManifestParser::OnPaymentMethodJsonParseSuccess,
                 weak_factory_.GetWeakPtr(), manifest_location,
                 base::Passed(&success_callback),
                 base::Passed(&invalid_data_callback)),
      base::Bind(&PaymentManifestParser::OnPaymentMethodJsonParseError,
                 weak_factory_.GetWeakPtr(),
                 base::Passed(&parse_error_callback)));
}

void PaymentManifestParser::ParseWebAppManifest(
    const std::string& content,
    WebAppCallback success_callback,
    JsonParseErrorCallback parse_error_callback,
    InvalidDataCallback invalid_data_callback) {
  DCHECK_GT(10U, parse_webapp_callback_counter_);
  parse_webapp_callback_counter_++;

  data_decoder::SafeJsonParser::Parse(
      content::ServiceManagerConnection::GetForProcess()->GetConnector(),
      content,
      base::Bind(&PaymentManifestParser::OnWebAppJsonParseSuccess,
                 weak_factory_.GetWeakPtr(), base::Passed(&success_callback),
                 base::Passed(&invalid_data_callback)),
      base::Bind(&PaymentManifestParser::OnWebAppJsonParseError,
                 weak_factory_.GetWeakPtr(),
                 base::Passed(&parse_error_callback)));
}

// static
bool PaymentManifestParser::ParsePaymentMethodManifestIntoVectors(
    std::unique_ptr<base::Value> value,
    const GURL& manifest_location,
    std::vector<GURL>* web_app_manifest_urls,
    std::vector<url::Origin>* supported_origins,
    bool* all_origins_supported) {
  DCHECK(web_app_manifest_urls);
  DCHECK(supported_origins);
  DCHECK(all_origins_supported);

  *all_origins_supported = false;

  std::unique_ptr<base::DictionaryValue> dict =
      base::DictionaryValue::From(std::move(value));
  if (!dict) {
    LOG(ERROR) << "Payment method manifest must be a JSON dictionary.";
    return false;
  }

  if (dict->HasKey(kDefaultApplications) &&
      !ParseDefaultApplications(dict.get(), manifest_location,
                                web_app_manifest_urls)) {
    return false;
  }

  if (dict->HasKey(kSupportedOrigins) &&
      !ParseSupportedOrigins(dict.get(), supported_origins,
                             all_origins_supported)) {
    web_app_manifest_urls->clear();
    return false;
  }

  return true;
}

// static
bool PaymentManifestParser::ParseWebAppManifestIntoVector(
    std::unique_ptr<base::Value> value,
    std::vector<WebAppManifestSection>* output) {
  std::unique_ptr<base::DictionaryValue> dict =
      base::DictionaryValue::From(std::move(value));
  if (!dict) {
    LOG(ERROR) << "Web app manifest must be a JSON dictionary.";
    return false;
  }

  base::ListValue* list = nullptr;
  if (!dict->GetList("related_applications", &list)) {
    LOG(ERROR) << "\"related_applications\" must be a list.";
    return false;
  }

  size_t related_applications_size = list->GetSize();
  for (size_t i = 0; i < related_applications_size; ++i) {
    base::DictionaryValue* related_application = nullptr;
    if (!list->GetDictionary(i, &related_application) || !related_application) {
      LOG(ERROR) << "\"related_applications\" must be a list of dictionaries.";
      output->clear();
      return false;
    }

    std::string platform;
    if (!related_application->GetString("platform", &platform) ||
        platform != "play") {
      continue;
    }

    if (output->size() >= kMaximumNumberOfItems) {
      LOG(ERROR) << "\"related_applications\" must contain at most "
                 << kMaximumNumberOfItems
                 << " entries with \"platform\": \"play\".";
      output->clear();
      return false;
    }

    const char* const kId = "id";
    const char* const kMinVersion = "min_version";
    const char* const kFingerprints = "fingerprints";
    if (!related_application->HasKey(kId) ||
        !related_application->HasKey(kMinVersion) ||
        !related_application->HasKey(kFingerprints)) {
      LOG(ERROR) << "Each \"platform\": \"play\" entry in "
                    "\"related_applications\" must contain \""
                 << kId << "\", \"" << kMinVersion << "\", and \""
                 << kFingerprints << "\".";
      return false;
    }

    WebAppManifestSection section;
    section.min_version = 0;

    if (!related_application->GetString(kId, &section.id) ||
        section.id.empty() || !base::IsStringASCII(section.id)) {
      LOG(ERROR) << "\"" << kId << "\" must be a non-empty ASCII string.";
      output->clear();
      return false;
    }

    std::string min_version;
    if (!related_application->GetString(kMinVersion, &min_version) ||
        min_version.empty() || !base::IsStringASCII(min_version) ||
        !base::StringToInt64(min_version, &section.min_version)) {
      LOG(ERROR) << "\"" << kMinVersion
                 << "\" must be a string convertible into a number.";
      output->clear();
      return false;
    }

    base::ListValue* fingerprints_list = nullptr;
    if (!related_application->GetList(kFingerprints, &fingerprints_list) ||
        fingerprints_list->empty() ||
        fingerprints_list->GetSize() > kMaximumNumberOfItems) {
      LOG(ERROR) << "\"" << kFingerprints
                 << "\" must be a non-empty list of at most "
                 << kMaximumNumberOfItems << " items.";
      output->clear();
      return false;
    }

    size_t fingerprints_size = fingerprints_list->GetSize();
    for (size_t j = 0; j < fingerprints_size; ++j) {
      base::DictionaryValue* fingerprint_dict = nullptr;
      std::string fingerprint_type;
      std::string fingerprint_value;
      if (!fingerprints_list->GetDictionary(j, &fingerprint_dict) ||
          !fingerprint_dict ||
          !fingerprint_dict->GetString("type", &fingerprint_type) ||
          fingerprint_type != "sha256_cert" ||
          !fingerprint_dict->GetString("value", &fingerprint_value) ||
          fingerprint_value.empty() ||
          !base::IsStringASCII(fingerprint_value)) {
        LOG(ERROR) << "Each entry in \"" << kFingerprints
                   << "\" must be a dictionary with \"type\": "
                      "\"sha256_cert\" and a non-empty ASCII string \"value\".";
        output->clear();
        return false;
      }

      std::vector<uint8_t> hash =
          FingerprintStringToByteArray(fingerprint_value);
      if (hash.empty()) {
        output->clear();
        return false;
      }

      section.fingerprints.push_back(hash);
    }

    output->push_back(section);
  }

  return true;
}

void PaymentManifestParser::OnPaymentMethodJsonParseSuccess(
    const GURL& manifest_location,
    PaymentMethodCallback success_callback,
    InvalidDataCallback invalid_data_callback,
    std::unique_ptr<base::Value> value) {
  DCHECK_LT(0U, parse_payment_callback_counter_);
  parse_payment_callback_counter_--;

  std::vector<GURL> web_app_manifest_urls;
  std::vector<url::Origin> supported_origins;
  bool all_origins_supported = false;
  if (ParsePaymentMethodManifestIntoVectors(
          std::move(value), manifest_location, &web_app_manifest_urls,
          &supported_origins, &all_origins_supported)) {
    std::move(success_callback)
        .Run(web_app_manifest_urls, supported_origins, all_origins_supported);
  } else {
    std::move(invalid_data_callback).Run();
  }
}

void PaymentManifestParser::OnPaymentMethodJsonParseError(
    JsonParseErrorCallback parse_error_callback,
    const std::string& error_message) {
  DCHECK_LT(0U, parse_payment_callback_counter_);
  parse_payment_callback_counter_--;

  LOG(ERROR) << error_message;
  std::move(parse_error_callback).Run();
}

void PaymentManifestParser::OnWebAppJsonParseSuccess(
    WebAppCallback success_callback,
    InvalidDataCallback invalid_data_callback,
    std::unique_ptr<base::Value> value) {
  DCHECK_LT(0U, parse_webapp_callback_counter_);
  parse_webapp_callback_counter_--;

  std::vector<WebAppManifestSection> manifest;
  if (ParseWebAppManifestIntoVector(std::move(value), &manifest))
    std::move(success_callback).Run(manifest);
  else
    std::move(invalid_data_callback).Run();
}

void PaymentManifestParser::OnWebAppJsonParseError(
    JsonParseErrorCallback parse_error_callback,
    const std::string& error_message) {
  DCHECK_LT(0U, parse_webapp_callback_counter_);
  parse_webapp_callback_counter_--;

  LOG(ERROR) << error_message;
  std::move(parse_error_callback).Run();
}

}  // namespace payments
