// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_PUBLIC_TEST_HTTP_SERVER_INFINITE_PENDING_RESPONSE_PROVIDER_H__
#define IOS_WEB_PUBLIC_TEST_HTTP_SERVER_INFINITE_PENDING_RESPONSE_PROVIDER_H__

#include "ios/web/public/test/http_server/html_response_provider.h"

#include "base/synchronization/condition_variable.h"
#import "base/test/ios/wait_util.h"

// Response provider that serves the page which never finishes loading.
class InfinitePendingResponseProvider : public HtmlResponseProvider {
 public:
  explicit InfinitePendingResponseProvider(const GURL& url);
  ~InfinitePendingResponseProvider() override;

  // Interrupt the current infinite request.
  // Must be called before the object is destroyed.
  void Abort();

  // HtmlResponseProvider overrides:
  bool CanHandleRequest(const Request& request) override;
  void GetResponseHeadersAndBody(
      const Request& request,
      scoped_refptr<net::HttpResponseHeaders>* headers,
      std::string* response_body) override;

  // Returns text that shows up after a navigation.
  static const char* GetPageText();

 private:
  // Returns a url for which this response provider will never reply.
  GURL GetInfinitePendingResponseUrl() const;

  // Main page URL that never finish loading.
  GURL url_;

  // Everything below is protected by lock_.
  mutable base::Lock lock_;
  std::atomic_bool aborted_;
  std::atomic_bool terminated_;
  base::ConditionVariable condition_variable_;
};

#endif  // IOS_WEB_PUBLIC_TEST_HTTP_SERVER_INFINITE_PENDING_RESPONSE_PROVIDER_H__
