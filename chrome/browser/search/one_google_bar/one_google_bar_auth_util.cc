// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/search/one_google_bar/one_google_bar_auth_util.h"

#include <stdint.h>
#include <utility>

#include "base/bind.h"
#include "base/callback.h"
#include "base/sha1.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "content/public/browser/browser_thread.h"
#include "net/cookies/cookie_store.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_getter.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace auth_util {

namespace {

std::string BuildAuthToken(int64_t timestamp,
                           const std::string& sapisid,
                           const std::string& origin) {
  std::string timestamp_str = base::Int64ToString(timestamp);
  std::string hash_binary =
      base::SHA1HashString(timestamp_str + " " + sapisid + " " + origin);
  std::string hash_str = base::ToLowerASCII(
      base::HexEncode(hash_binary.data(), hash_binary.size()));
  return "SAPISIDHASH " + timestamp_str + "_" + hash_str;
}

void CookiesReceived(const GURL& google_base_url,
                     base::OnceCallback<void(const std::string&)> callback,
                     const net::CookieList& cookies) {
  std::string auth_token;
  for (const net::CanonicalCookie& cookie : cookies) {
    if (cookie.Name() == "SAPISID") {
      int64_t timestamp =
          (base::Time::Now() - base::Time::UnixEpoch()).InSeconds();
      url::Origin origin(google_base_url);
      auth_token =
          BuildAuthToken(timestamp, cookie.Value(), origin.Serialize());
      break;
    }
  }
  content::BrowserThread::PostTask(
      content::BrowserThread::UI, FROM_HERE,
      base::BindOnce(std::move(callback), auth_token));
}

void GetAuthTokenOnIOThread(
    scoped_refptr<net::URLRequestContextGetter> request_context_getter,
    const GURL& google_base_url,
    base::OnceCallback<void(const std::string&)> callback) {
  net::CookieStore* cookie_store =
      request_context_getter->GetURLRequestContext()->cookie_store();
  net::CookieOptions options;
  options.set_include_httponly();
  cookie_store->GetCookieListWithOptionsAsync(
      google_base_url, options,
      base::BindOnce(&CookiesReceived, google_base_url, std::move(callback)));
}

}  // namespace

void GetAuthToken(net::URLRequestContextGetter* request_context_getter,
                  const GURL& google_base_url,
                  base::OnceCallback<void(const std::string&)> callback) {
  content::BrowserThread::PostTask(
      content::BrowserThread::IO, FROM_HERE,
      base::BindOnce(
          GetAuthTokenOnIOThread,
          scoped_refptr<net::URLRequestContextGetter>(request_context_getter),
          google_base_url, std::move(callback)));
}

}  // namespace auth_util
