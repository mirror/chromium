// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CrossOriginPreflightResultCache_h
#define CrossOriginPreflightResultCache_h

#include <memory>
#include "base/time/time.h"
#include "content/common/service_worker/service_worker_types.h"
#include "content/public/common/resource_response.h"
#include "net/http/http_request_headers.h"
#include "url/gurl.h"

namespace content {

namespace {

typedef std::pair<std::string, GURL> OriginURL;

struct IgnoreCaseHash {
  size_t operator()(const std::string& value) const {
    std::string lcase = value;
    std::transform(lcase.begin(), lcase.end(), lcase.begin(), ::tolower);

    return std::hash<std::string>{}(lcase);
  }
};

struct IgnoreCaseEqual {
  bool operator()(const std::string& left, const std::string& right) const {
    return left.size() == right.size() &&
           std::equal(left.begin(), left.end(), right.begin(),
                      [](char a, char b) { return tolower(a) == tolower(b); });
  }
};

struct OriginURLHash {
  size_t operator()(const OriginURL& origin_url) const {
    return std::hash<std::string>()(origin_url.first) ^
           (std::hash<std::string>()(origin_url.second.spec()) << 1);
  }
};

}  // namespace

// Represents an entry of the CORS-preflight cache.
// See https://fetch.spec.whatwg.org/#concept-cache.
class CrossOriginPreflightResultCacheItem {
  // TODO(hintzed): WTF_MAKE_NONCOPYABLE(CrossOriginPreflightResultCacheItem);
  // TODO(hintzed): USING_FAST_MALLOC(CrossOriginPreflightResultCacheItem);

 public:
  typedef std::unordered_set<std::string, IgnoreCaseHash, IgnoreCaseEqual>
      IgnoreCaseStringSet;

  explicit CrossOriginPreflightResultCacheItem(content::FetchCredentialsMode);

  ~CrossOriginPreflightResultCacheItem();

  bool Parse(const ResourceResponse&, std::string& error_description);
  bool AllowsCrossOriginMethod(const std::string&,
                               std::string& error_description) const;
  bool AllowsCrossOriginHeaders(const net::HttpRequestHeaders&,
                                std::string& error_description) const;
  bool AllowsRequest(content::FetchCredentialsMode,
                     const std::string& method,
                     const net::HttpRequestHeaders& request_headers) const;

 private:
  // FIXME: A better solution to holding onto the absolute expiration time might
  // be to start a timer for the expiration delta that removes this from the
  // cache when it fires.
  double absolute_expiry_time_;

  // Corresponds to the fields of the CORS-preflight cache with the same name.
  bool credentials_;

  IgnoreCaseStringSet methods_;
  IgnoreCaseStringSet headers_;
};

class CrossOriginPreflightResultCache {
  //  WTF_MAKE_NONCOPYABLE(CrossOriginPreflightResultCache);
  //  USING_FAST_MALLOC(CrossOriginPreflightResultCache);

 public:
  static CrossOriginPreflightResultCache& Shared();

  ~CrossOriginPreflightResultCache();

  void AppendEntry(const std::string& origin,
                   const GURL&,
                   std::unique_ptr<CrossOriginPreflightResultCacheItem>);

  bool CanSkipPreflight(const std::string& origin,
                        const GURL&,
                        content::FetchCredentialsMode,
                        const std::string& method,
                        const std::string& request_headers);

  static double CurrentTime() { return base::Time::Now().ToDoubleT(); }

 private:
  CrossOriginPreflightResultCache();

  typedef std::unordered_map<
      OriginURL,
      std::unique_ptr<CrossOriginPreflightResultCacheItem>,
      OriginURLHash>
      CrossOriginPreflightResultHashMap;

  CrossOriginPreflightResultHashMap preflight_hash_map_;
};

}  // namespace content

#endif
