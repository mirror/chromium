// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/language/content/util/geocoding_request.h"

#include "base/json/json_reader.h"
#include "base/strings/stringprintf.h"
#include "base/values.h"
#include "google_apis/google_api_keys.h"
#include "net/base/escape.h"
#include "net/base/load_flags.h"
#include "net/http/http_status_code.h"
#include "net/url_request/url_request_context_getter.h"
#include "net/url_request/url_request_status.h"

namespace language {
namespace {

// Request string constants.
const char kLatLngString[] = "latlng";
const char kKeyString[] = "key";
const char kResultTypeString[] = "result_type";
const char kAdminLevelString[] = "administrative_area_level_1";

// Response json keys.
const char kStatusString[] = "status";
const char kResultsString[] = "results";
const char kAddressComponentsString[] = "address_components";
const char kLongNameString[] = "long_name";
const char kTypesString[] = "types";

GURL ConstructRequestUrl(const GURL& url, double latitude, double longitude) {
  DCHECK(url.is_valid());
  std::string query(url.query());

  query += base::StringPrintf("%s=%f,%f", kLatLngString, latitude, longitude);
  query += base::StringPrintf("&%s=%s", kResultTypeString, kAdminLevelString);

  std::string api_key = google_apis::GetAPIKey();
  if (!api_key.empty()) {
    query += "&";
    query += kKeyString;
    query += "=";
    query += net::EscapeQueryParamValue(api_key, true);
  }

  GURL::Replacements replacements;
  replacements.SetQueryStr(query);
  return url.ReplaceComponents(replacements);
}

// Parse the geocoding response and get the admin district.
// If error happens, will resolve in unknown admin district.
// This will also indicate whether the parse has errors.
// TODO(renjieliu): Consider log the error to uma.
bool ParseGeodingResponseHasError(const std::string& response_body,
                                  std::string* admin_district) {
  if (response_body.empty()) {
    LOG(ERROR) << "Geocoding empty response.";
    return true;
  }
  // Parse the response, ignoring comments.
  std::string error_msg;
  std::unique_ptr<base::Value> response_value =
      base::JSONReader::ReadAndReturnError(response_body, base::JSON_PARSE_RFC,
                                           nullptr, &error_msg);
  if (response_value == nullptr) {
    LOG(ERROR) << "JSONReader failed to parse geocoding response: "
               << error_msg;
    return true;
  }

  const base::DictionaryValue* response_object = nullptr;
  if (!response_value->GetAsDictionary(&response_object)) {
    LOG(ERROR) << "Geocoding response unexpected response type.";
    return true;
  }

  std::string status;
  if (!response_object->GetStringWithoutPathExpansion(kStatusString, &status)) {
    LOG(ERROR) << "Geocoding response does not have status.";
    return true;
  }

  if (status != "OK") {
    LOG(ERROR) << "Geocoding status not ok.";
    return true;
  }

  const base::ListValue* results = nullptr;
  if (!response_object->GetListWithoutPathExpansion(kResultsString, &results) ||
      results->GetSize() < 1) {
    LOG(ERROR) << "Geocoding does not have result.";
    return true;
  }

  const base::DictionaryValue* first_value_result = nullptr;
  if (!results->GetDictionary(0, &first_value_result)) {
    LOG(ERROR) << "Geocoding cannot get the result.";
  }

  const base::ListValue* address_components = nullptr;
  if (!first_value_result->GetListWithoutPathExpansion(kAddressComponentsString,
                                                       &address_components)) {
    LOG(ERROR) << "Geocoding cannot find address components.";
    return true;
  }

  // Iterate through the address_components and find the longname if
  // corresponding type has administrative_area_level_1.
  for (size_t i = 0; i < address_components->GetSize(); ++i) {
    const base::DictionaryValue* admin_districts = nullptr;
    if (address_components->GetDictionary(i, &admin_districts)) {
      const base::ListValue* types = nullptr;
      if (admin_districts->GetListWithoutPathExpansion(kTypesString, &types)) {
        std::string type;
        if (types->GetString(0, &type) && type == kAdminLevelString) {
          // Try to get the admin district.
          if (admin_districts->GetStringWithoutPathExpansion(kLongNameString,
                                                             admin_district)) {
            return false;
          }
        }
      }
    }
  }

  LOG(ERROR) << "Geocoding cannot find admin district.";
  return true;
}

}  // namespace

GeocodingRequest::GeocodingRequest(
    net::URLRequestContextGetter* url_context_getter,
    const GURL& service_url,
    const double latitude,
    const double longitude)
    : url_context_getter_(url_context_getter),
      service_url_(service_url),
      latitude_(latitude),
      longitude_(longitude) {
  request_url_ = ConstructRequestUrl(service_url_, latitude_, longitude_);
}

GeocodingRequest::~GeocodingRequest() {
  url_fetcher_.reset();
  callback_.Reset();
}

void GeocodingRequest::MakeRequest(GeocodingResponseCallback callback) {
  callback_ = callback;
  StartRequest();
}

void GeocodingRequest::StartRequest() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  url_fetcher_ =
      net::URLFetcher::Create(request_url_, net::URLFetcher::GET, this);
  url_fetcher_->SetRequestContext(url_context_getter_.get());
  url_fetcher_->SetLoadFlags(net::LOAD_BYPASS_CACHE | net::LOAD_DISABLE_CACHE |
                             net::LOAD_DO_NOT_SAVE_COOKIES |
                             net::LOAD_DO_NOT_SEND_COOKIES |
                             net::LOAD_DO_NOT_SEND_AUTH_DATA);
  url_fetcher_->Start();
}

void GeocodingRequest::OnURLFetchComplete(const net::URLFetcher* source) {
  DCHECK_EQ(url_fetcher_.get(), source);
  net::URLRequestStatus status = source->GetStatus();
  int response_code = source->GetResponseCode();
  const bool server_error = !status.is_success() || response_code != 200;

  // TODO(renjieliu): Implement retry logic.

  std::string data;
  source->GetResponseAsString(&data);
  std::string admin_district = "unknown";

  bool parse_error = true;
  if (!server_error) {
    parse_error = ParseGeodingResponseHasError(data, &admin_district);
  }

  callback_.Run(admin_district, server_error || parse_error);
}

}  // namespace language