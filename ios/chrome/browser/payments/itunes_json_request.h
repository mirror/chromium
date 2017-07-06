// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_PAYMENTS_ITUNES_JSON_REQUEST_H_
#define IOS_CHROME_BROWSER_PAYMENTS_ITUNES_JSON_REQUEST_H_

#include <memory>
#include <string>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "net/url_request/url_fetcher_delegate.h"

class GURL;

namespace base {
class DictionaryValue;
class Value;
}  // namespace base

namespace net {
class URLFetcher;
class URLRequestContextGetter;
}  // namespace net

namespace app_list {

// A class that fetches a JSON formatted response from a server and uses a
// sandboxed utility process to parse it to a DictionaryValue.
class JSONResponseFetcher : public net::URLFetcherDelegate {
 public:
  // Callback to pass back the parsed json dictionary returned from the server.
  // Invoked with NULL if there is an error.
  typedef base::Callback<void(std::unique_ptr<base::DictionaryValue>)> Callback;

  JSONResponseFetcher(const Callback& callback,
                      net::URLRequestContextGetter* context_getter);
  ~JSONResponseFetcher() override;

  // Starts to fetch results for the given |query_url|.
  void Start(const GURL& query_url);
  void Stop();

 private:
  // Callbacks for SafeJsonParser.
  void OnJsonParseSuccess(std::unique_ptr<base::Value> parsed_json);
  void OnJsonParseError(const std::string& error);

  // net::URLFetcherDelegate overrides:
  void OnURLFetchComplete(const net::URLFetcher* source) override;

  Callback callback_;
  net::URLRequestContextGetter* context_getter_;

  std::unique_ptr<net::URLFetcher> fetcher_;
  base::WeakPtrFactory<JSONResponseFetcher> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(JSONResponseFetcher);
};

}  // namespace app_list

#endif  // IOS_CHROME_BROWSER_PAYMENTS_ITUNES_JSON_REQUEST_H_
