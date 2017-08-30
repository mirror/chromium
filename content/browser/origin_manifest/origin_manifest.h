// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_ORIGIN_MANIFEST_ORIGIN_MANIFEST_H_
#define CONTENT_BROWSER_ORIGIN_MANIFEST_ORIGIN_MANIFEST_H_

#include <map>
#include <vector>

#include "base/time/time.h"
#include "content/common/content_export.h"

namespace base {
class Value;
}

namespace net {
class HttpRequestHeaders;
class HttpResponseHeaders;
}  // namespace net

namespace content {

class CONTENT_EXPORT OriginManifest {
 public:
  OriginManifest(const OriginManifest& om);
  virtual ~OriginManifest();

  // Parses the input JSON and translates the stucture into a OriginManifest
  // object. Returns nullptr on invalid JSON.
  static std::unique_ptr<OriginManifest> CreateOriginManifest(
      std::string origin,
      std::string version,
      std::string json);

  void ApplyToRequest(net::HttpRequestHeaders* request_headers) const;
  void ApplyToRedirect(net::HttpResponseHeaders* response_headers) const;
  void ApplyToResponse(net::HttpResponseHeaders* response_headers) const;

  void Revoke() const;

  const std::string Origin() const { return origin_; }
  const std::string Version() const { return version_; }
  const std::string JSON() const { return json_; }
  const base::Time& CreationDate() const { return creation_date_; }
  const base::Time& ExpiryDate() const { return expiry_date_; }

  bool IsExpired(const base::Time& current) const {
    return !expiry_date_.is_null() && current >= expiry_date_;
  }

  void SetCreationDate(base::Time new_creation_date);
  void SetExpiryDate(base::Time new_expiry_date);

  const char* GetNameForLogging();

 private:
  enum SUPPORTED_KEYS {
    STRICT_TRANSPORT_SECURITY_KEY,
    CONTENT_SECURITY_POLICY_KEY,
    CORS_PREFLIGHT,
    UNKNOWN_KEY,
  };
  OriginManifest(std::string origin, std::string version, std::string json);

  bool AddBaseline(std::string key, const base::Value& value);
  bool AddFallback(std::string key, const base::Value& value);

  std::string origin_;
  std::string version_;
  std::string json_;
  base::Time creation_date_;
  base::Time expiry_date_;

  std::map<std::string, std::vector<std::string>> baseline;
  std::map<std::string, std::vector<std::string>> fallback;
};

}  // namespace content

#endif  // CONTENT_BROWSER_ORIGIN_MANIFEST_ORIGIN_MANIFEST_H_
