// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/origin_manifest/origin_manifest.h"

#include "base/json/json_reader.h"
#include "base/strings/string_util.h"
#include "base/values.h"
#include "content/public/browser/navigation_handle.h"
#include "net/http/http_response_headers.h"

namespace content {

OriginManifest::OriginManifest(const OriginManifest& om)
    : OriginManifest(om.Origin(), om.Version(), om.JSON()) {
  SetCreationDate(om.CreationDate());
  SetExpiryDate(om.ExpiryDate());
}

OriginManifest::OriginManifest(std::string origin,
                               std::string version,
                               std::string json)
    : origin_(origin),
      version_(version),
      json_(json),
      creation_date_(base::Time::Now()),
      expiry_date_(base::Time::Now()) {}

OriginManifest::~OriginManifest() {}

// static
std::unique_ptr<OriginManifest> OriginManifest::CreateOriginManifest(
    std::string origin,
    std::string version,
    std::string json) {
  std::unique_ptr<base::DictionaryValue> directives_dict =
      base::DictionaryValue::From(
          base::JSONReader::Read(json, base::JSON_ALLOW_TRAILING_COMMAS));

  if (directives_dict == nullptr)
    return nullptr;

  OriginManifest* origin_manifest = new OriginManifest(origin, version, json);

  for (base::DictionaryValue::Iterator it(*(directives_dict.get()));
       !it.IsAtEnd(); it.Advance()) {
    const std::string& key = it.key();
    if (base::EndsWith(it.key(), "-fallback", base::CompareCase::SENSITIVE)) {
      // we expect the actual keyword so we remove "-fallback"
      origin_manifest->AddFallback(key.substr(0, key.size() - 9), it.value());
    } else {
      origin_manifest->AddBaseline(key, it.value());
    }
  }

  return std::unique_ptr<OriginManifest>(origin_manifest);
}

void OriginManifest::ApplyToRequest(
    net::HttpRequestHeaders* request_headers) const {
  // called in OriginManifestResourceHandler::OnWillStart, so only use the extra
  // request handlers
  // nothing to at the moment...
}

void OriginManifest::ApplyToRedirect(
    net::HttpResponseHeaders* response_headers) const {
  // At the moment there is nothing I know of makes sense to apply. It is never
  // used iirc
}

void OriginManifest::ApplyToResponse(
    net::HttpResponseHeaders* response_headers) const {
  for (std::map<std::string, std::vector<std::string>>::const_iterator iter =
           fallback.begin();
       iter != fallback.end(); iter++) {
    if (!response_headers->HasHeader(iter->first)) {
      for (std::string value : iter->second)
        response_headers->AddHeader((iter->first) + ": " + value);
    }
  }

  for (std::map<std::string, std::vector<std::string>>::const_iterator iter =
           baseline.begin();
       iter != baseline.end(); iter++) {
    for (std::string value : iter->second)
      response_headers->AddHeader((iter->first) + ": " + value);
  }
}

void OriginManifest::Revoke() const {
  // currently we only support CSP, so nothing to do here for now
}

const char* OriginManifest::GetNameForLogging() {
  return "OriginManifest";
}

bool OriginManifest::AddBaseline(std::string key, const base::Value& value) {
  // check against a list of supported keywords and add only then
  if (key == "content-security-policy" && value.is_string()) {
    baseline[key].push_back(value.GetString());
  }
  return false;
}

bool OriginManifest::AddFallback(std::string key, const base::Value& value) {
  // check against a list of supported keywords and add only then
  if (key == "content-security-policy" && value.is_string()) {
    fallback[key].push_back(value.GetString());
    return true;
  }
  return false;
}

void OriginManifest::SetCreationDate(base::Time new_creation_date) {
  creation_date_ = new_creation_date;
}

void OriginManifest::SetExpiryDate(base::Time new_expiry_date) {
  expiry_date_ = new_expiry_date;
}

}  // namespace content
