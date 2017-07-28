// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CROSS_ORIGIN_PREFLIGHT_RESULT_CACHE_H_
#define CROSS_ORIGIN_PREFLIGHT_RESULT_CACHE_H_

#include <memory>
#include "base/hash.h"
#include "base/macros.h"
#include "base/time/time.h"
#include "content/common/service_worker/service_worker_types.h"
#include "content/public/common/resource_response.h"
#include "content/renderer/net/str_util.h"
#include "net/http/http_request_headers.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace content {

namespace {

typedef std::pair<url::Origin, GURL> OriginURL;

struct OriginURLHash {
  size_t operator()(const OriginURL& origin_url) const {
    std::string origin = origin_url.first.Serialize();
    std::string& ref = origin;
    return base::HashInts32(base::Hash(ref),
                            base::Hash(origin_url.second.spec()));
  }
};

struct OriginURLEqual {
  bool operator()(const OriginURL& left, const OriginURL& right) const {
    return left.first.Serialize() == right.first.Serialize() &&
           left.second.spec() == right.second.spec();
  }
};

}  // namespace

// Represents an entry of the CORS-preflight cache.
// See https://fetch.spec.whatwg.org/#concept-cache.
class CONTENT_EXPORT CrossOriginPreflightResultCacheItem {
 public:
  static std::unique_ptr<CrossOriginPreflightResultCacheItem> Create(
      const FetchCredentialsMode credentials_mode,
      const ResourceResponseHead& head,
      std::string& error_description);

  ~CrossOriginPreflightResultCacheItem();

  // True if |method| is either a CORS-safelisted method
  // (https://fetch.spec.whatwg.org/#cors-safelisted-method) or explicitly
  // allowed according to a cached preflight response.
  // Populates |error_description| when returning false.
  bool AllowsCrossOriginMethod(const std::string& method,
                               std::string& error_description) const;

  // True if none of the header names in |request_headers| is a forbidden header
  // name (https://fetch.spec.whatwg.org/#forbidden-header-name) and all header
  // names in |request_headers| are either explicitly allowed according to a
  // cached preflight response or are CORS-safelisted request-header names
  // (https://fetch.spec.whatwg.org/#cors-safelisted-request-header). Return
  // false otherwise.
  // Populates |error_description| when returning false.
  bool AllowsCrossOriginHeaders(const net::HttpRequestHeaders& request_headers,
                                std::string& error_description) const;

  // Returns true if all of the following conditions hold and false otherwise:
  //   - The cache entry is not expired,
  //   - credentials are either explicitly allowed or not included,
  //   - the method is allowed according to AllowsCrossOriginMethod,
  //   - all headers are allowed according to AllowsCrossOriginHeaders.
  bool AllowsRequest(FetchCredentialsMode credentials_mode,
                     const std::string& method,
                     const net::HttpRequestHeaders& request_headers) const;

 private:
  explicit CrossOriginPreflightResultCacheItem(
      FetchCredentialsMode credentials_mode);

  inline double CurrentTime() const { return base::Time::Now().ToDoubleT(); }

  bool Parse(const ResourceResponseHead& head, std::string& error_description);

  // FIXME: A better solution to holding onto the absolute expiration time might
  // be to start a timer for the expiration delta that removes this from the
  // cache when it fires.
  double absolute_expiry_time_;

  // Corresponds to the fields of the CORS-preflight cache with the same name.
  bool credentials_;

  IgnoreCaseStringSet methods_;
  IgnoreCaseStringSet headers_;

  DISALLOW_COPY_AND_ASSIGN(CrossOriginPreflightResultCacheItem);
};

class CONTENT_EXPORT CrossOriginPreflightResultCache {
  //  WTF_MAKE_NONCOPYABLE(CrossOriginPreflightResultCache);
  //  USING_FAST_MALLOC(CrossOriginPreflightResultCache);

 public:
  static CrossOriginPreflightResultCache& Shared();

  ~CrossOriginPreflightResultCache();

  void AppendEntry(const url::Origin& origin,
                   const GURL& url,
                   std::unique_ptr<CrossOriginPreflightResultCacheItem> item);

  bool CanSkipPreflight(const url::Origin& origin,
                        const GURL&,
                        FetchCredentialsMode,
                        const std::string& method,
                        const std::string& request_headers);

 protected:
  // Protected for tests:
  CrossOriginPreflightResultCache();

 private:
  typedef std::unordered_map<
      OriginURL,
      std::unique_ptr<CrossOriginPreflightResultCacheItem>,
      OriginURLHash,
      OriginURLEqual>
      CrossOriginPreflightResultHashMap;

  CrossOriginPreflightResultHashMap preflight_hash_map_;

  DISALLOW_COPY_AND_ASSIGN(CrossOriginPreflightResultCache);
};

}  // namespace content

#endif  // CROSS_ORIGIN_PREFLIGHT_RESULT_CACHE_H_
