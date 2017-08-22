// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_FRAME_HOST_ORIGIN_MANIFEST_ORIGIN_MANIFEST_H_
#define CONTENT_BROWSER_FRAME_HOST_ORIGIN_MANIFEST_ORIGIN_MANIFEST_H_

#include <map>
#include <vector>

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
  virtual ~OriginManifest();

  // Parses the input JSON and translates the stucture into a OriginManifest
  // object. Returns nullptr on invalid JSON.
  static std::unique_ptr<OriginManifest> CreateOriginManifest(std::string json);

  void ApplyToRequest(net::HttpRequestHeaders* request_headers) const;
  void ApplyToRedirect(net::HttpResponseHeaders* response_headers) const;
  void ApplyToResponse(net::HttpResponseHeaders* response_headers) const;

  void Revoke() const;

  const char* GetNameForLogging();

 private:
  OriginManifest();

  bool AddBaseline(std::string key, const base::Value& value);
  bool AddFallback(std::string key, const base::Value& value);

  std::map<std::string, std::vector<std::string>> baseline;
  std::map<std::string, std::vector<std::string>> fallback;
};

}  // namespace content

#endif  // CONTENT_BROWSER_FRAME_HOST_ORIGIN_MANIFEST_ORIGIN_MANIFEST_H_
