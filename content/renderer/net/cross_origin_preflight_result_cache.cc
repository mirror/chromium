// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/net/cross_origin_preflight_result_cache.h"

#include "base/macros.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "content/renderer/net/cross_origin_access_control.h"
#include "content/renderer/render_thread_impl.h"

namespace content {

namespace {

// These values are at the discretion of the user agent.

constexpr unsigned kDefaultPreflightCacheTimeoutSeconds = 5;

// Should be short enough to minimize the risk of using a poisoned cache after
// switching to a secure network.
constexpr unsigned kMaxPreflightCacheTimeoutSeconds = 600;

bool ParseAccessControlMaxAge(const std::string& string,
                              unsigned& expiry_delta) {
  // FIXME: this will not do the correct thing for a number starting with a '+'
  // TODO(hintzed): Does the above FIXME still hold?
  return base::StringToUint(string, &expiry_delta);
}

void AddToAccessControlAllowList(const std::string& string,
                                 std::string::size_type start,
                                 std::string::size_type end,
                                 IgnoreCaseStringSet& set) {
  std::string trimmed_string = string.substr(start, end - start + 1);

  base::TrimWhitespaceASCII(trimmed_string, base::TRIM_ALL, &trimmed_string);

  // only white space
  if (trimmed_string.empty())
    return;

  set.insert(trimmed_string);
}

bool ParseAccessControlAllowList(const std::string& string,
                                 IgnoreCaseStringSet& set) {
  std::string::size_type start = 0;
  std::string::size_type end;

  while ((end = string.find(',', start)) != std::string::npos) {
    if (start != end)
      AddToAccessControlAllowList(string, start, end - 1, set);
    start = end + 1;
  }

  if (start != string.length())
    AddToAccessControlAllowList(string, start, string.length() - 1, set);

  return true;
}

static bool IsMainThread() {
  // TODO(hintzed): How to have this return true for UnitTest?
  // return !!RenderThreadImpl::current();

  return true;
}

}  // namespace

CrossOriginPreflightResultCacheItem::CrossOriginPreflightResultCacheItem(
    content::FetchCredentialsMode credentials_mode)
    : absolute_expiry_time_(0),
      credentials_(
          CrossOriginAccessControl::ShouldTreatCredentialsModeAsInclude(
              credentials_mode)) {}

CrossOriginPreflightResultCache::CrossOriginPreflightResultCache(){};

CrossOriginPreflightResultCacheItem::~CrossOriginPreflightResultCacheItem() {}

// static
std::unique_ptr<CrossOriginPreflightResultCacheItem>
CrossOriginPreflightResultCacheItem::Create(
    const content::FetchCredentialsMode credentials_mode,
    const ResourceResponseHead& head,
    std::string& error_description) {
  std::unique_ptr<CrossOriginPreflightResultCacheItem> item = base::WrapUnique(
      new CrossOriginPreflightResultCacheItem(credentials_mode));

  if (!item->Parse(head, error_description))
    return nullptr;

  return item;
}

bool CrossOriginPreflightResultCacheItem::Parse(
    const ResourceResponseHead& head,
    std::string& error_description) {
  DCHECK(methods_.empty());
  DCHECK(headers_.empty());

  std::string allowed_methods;
  head.headers.get()->GetNormalizedHeader(
      CrossOriginAccessControl::kAccessControlAllowMethods, &allowed_methods);

  if (!ParseAccessControlAllowList(allowed_methods, methods_)) {
    error_description =
        "Cannot parse Access-Control-Allow-Methods response header field in "
        "preflight response.";

    return false;
  }

  std::string allowed_headers;
  head.headers.get()->GetNormalizedHeader(
      CrossOriginAccessControl::kAccessControlAllowHeaders, &allowed_headers);
  if (!ParseAccessControlAllowList(allowed_headers, headers_)) {
    error_description =
        "Cannot parse Access-Control-Allow-Headers response header field in "
        "preflight response.";
    return false;
  }

  unsigned expiry_delta;
  std::string max_age;
  head.headers.get()->GetNormalizedHeader(
      CrossOriginAccessControl::kAccessControlMaxAge, &max_age);
  if (ParseAccessControlMaxAge(max_age, expiry_delta)) {
    if (expiry_delta > kMaxPreflightCacheTimeoutSeconds)
      expiry_delta = kMaxPreflightCacheTimeoutSeconds;
  } else {
    expiry_delta = kDefaultPreflightCacheTimeoutSeconds;
  }

  absolute_expiry_time_ = CurrentTime() + expiry_delta;

  return true;
}

bool CrossOriginPreflightResultCacheItem::AllowsCrossOriginMethod(
    const std::string& method,
    std::string& error_description) const {
  if (methods_.find(method) != methods_.end() ||
      CrossOriginAccessControl::IsCORSSafelistedMethod(method))
    return true;

  error_description =
      "Method " + method +
      " is not allowed by Access-Control-Allow-Methods in preflight response.";
  return false;
}

bool CrossOriginPreflightResultCacheItem::AllowsCrossOriginHeaders(
    const net::HttpRequestHeaders& request_headers,
    std::string& error_description) const {
  for (net::HttpRequestHeaders::Iterator it(request_headers); it.GetNext();) {
    if (headers_.find(it.name()) == headers_.end() &&
        !CrossOriginAccessControl::IsCORSSafelistedHeader(it.name(),
                                                          it.value()) &&
        !CrossOriginAccessControl::IsForbiddenHeaderName(it.name())) {
      error_description = "Request header field " + it.name() +
                          " is not allowed by Access-Control-Allow-Headers in "
                          "preflight response.";

      return false;
    }
  }
  return true;
}

bool CrossOriginPreflightResultCacheItem::AllowsRequest(
    content::FetchCredentialsMode credentials_mode,
    const std::string& method,
    const net::HttpRequestHeaders& request_headers) const {
  std::string ignored_explanation;

  if (absolute_expiry_time_ < CurrentTime())
    return false;

  if (!credentials_ &&
      CrossOriginAccessControl::ShouldTreatCredentialsModeAsInclude(
          credentials_mode))
    return false;

  if (!AllowsCrossOriginMethod(method, ignored_explanation))
    return false;

  if (!AllowsCrossOriginHeaders(request_headers, ignored_explanation))
    return false;

  return true;
}

CrossOriginPreflightResultCache::~CrossOriginPreflightResultCache(){};

CrossOriginPreflightResultCache& CrossOriginPreflightResultCache::Shared() {
  DCHECK(IsMainThread());

  // TODO(hintzed): This is probably a bad idea in terms of memory leakage?
  CR_DEFINE_STATIC_LOCAL(content::CrossOriginPreflightResultCache, instance,
                         ());

  return instance;
}

void CrossOriginPreflightResultCache::AppendEntry(
    const url::Origin& origin,
    const GURL& url,
    std::unique_ptr<CrossOriginPreflightResultCacheItem> preflight_result) {
  DCHECK(IsMainThread());
  preflight_hash_map_[std::make_pair(origin, url)] =
      std::move(preflight_result);
}

bool CrossOriginPreflightResultCache::CanSkipPreflight(
    const url::Origin& origin,
    const GURL& url,
    content::FetchCredentialsMode credentials_mode,
    const std::string& method,
    const std::string& request_headers) {
  DCHECK(IsMainThread());

  net::HttpRequestHeaders headers;
  headers.AddHeadersFromString(request_headers);

  CrossOriginPreflightResultHashMap::iterator cache_it =
      preflight_hash_map_.find(std::make_pair(origin, url));

  if (cache_it == preflight_hash_map_.end())
    return false;

  if (cache_it->second->AllowsRequest(credentials_mode, method, headers))
    return true;

  preflight_hash_map_.erase(cache_it);

  return false;
}

}  // namespace content
