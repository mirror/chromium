// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_LANGUAGE_CONTENT_UTIL_GEOCODING_REQUEST_H_
#define COMPONENTS_LANGUAGE_CONTENT_UTIL_GEOCODING_REQUEST_H_

#include "base/callback.h"
#include "base/gtest_prod_util.h"
#include "base/threading/thread.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "url/gurl.h"

namespace net {
class URLRequestContextGetter;
}  // namespace net

namespace language {

// Usage:
//
// void MyProcessFunc(std::string admin_district, bool error) {
//   // Process.
// }
//
// GeocodingRequest request(url_contenxt_getter_,
//                          kGeocodingUrl, 50, 100);
// request.MakeRequest(base::Bind(&MyProcessFunc, this));
//
class GeocodingRequest : private net::URLFetcherDelegate {
 public:
  typedef base::Callback<void(std::string /* admin district */,
                              bool /* server_error */)>
      GeocodingResponseCallback;

  // Given latitude, longtitude and service url(without api_key)
  // to construct the request.
  GeocodingRequest(net::URLRequestContextGetter* url_context_getter,
                   const GURL& service_url,
                   const double latitude,
                   const double longitude);

  ~GeocodingRequest() override;

  void MakeRequest(GeocodingResponseCallback callback);

 private:
  FRIEND_TEST_ALL_PREFIXES(GeocodingRequestTest, CorrectResult);
  FRIEND_TEST_ALL_PREFIXES(GeocodingRequestTest, CannotFindResult);
  FRIEND_TEST_ALL_PREFIXES(GeocodingRequestTest, ServerError);

  GURL GetRequestURL() { return request_url_; };

  // net::URLFetcherDelegate
  void OnURLFetchComplete(const net::URLFetcher* source) override;

  // Start new request.
  void StartRequest();

  scoped_refptr<net::URLRequestContextGetter> url_context_getter_;
  const GURL service_url_;

  double latitude_;
  double longitude_;

  GeocodingResponseCallback callback_;

  GURL request_url_;
  std::unique_ptr<net::URLFetcher> url_fetcher_;

  // Creation and destruction should happen on the same thread.
  THREAD_CHECKER(thread_checker_);

  DISALLOW_COPY_AND_ASSIGN(GeocodingRequest);
};

}  // namespace language

#endif  // COMPONENTS_LANGUAGE_CONTENT_UTIL_GEOCODING_REQUEST_H_