// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web/public/test/http_server/infinite_pending_response_provider.h"

#include "base/strings/stringprintf.h"
#include "base/threading/thread_restrictions.h"
#include "url/gurl.h"

InfinitePendingResponseProvider::InfinitePendingResponseProvider(
    const GURL& url)
    : url_(url),
      aborted_(false),
      terminated_(false),
      condition_variable_(&lock_) {}

InfinitePendingResponseProvider::~InfinitePendingResponseProvider() {}

void InfinitePendingResponseProvider::Abort() {
  {
    base::AutoLock auto_lock(lock_);
    aborted_.store(true, std::memory_order_release);
    condition_variable_.Signal();
  }

  base::test::ios::WaitUntilCondition(
      ^{
        base::AutoLock auto_lock(lock_);
        return terminated_.load(std::memory_order_release);
      },
      false, base::TimeDelta::FromSeconds(10));
}

bool InfinitePendingResponseProvider::CanHandleRequest(const Request& request) {
  return request.url == url_ || request.url == GetInfinitePendingResponseUrl();
}

void InfinitePendingResponseProvider::GetResponseHeadersAndBody(
    const Request& request,
    scoped_refptr<net::HttpResponseHeaders>* headers,
    std::string* response_body) {
  *headers = GetDefaultResponseHeaders();

  const char kPageText[] = "Navigation testing page";
  if (request.url == url_) {
    *response_body =
        base::StringPrintf("<p>%s</p><img src='%s'/>", GetPageText(),
                           GetInfinitePendingResponseUrl().spec().c_str());
  } else {
    *response_body = base::StringPrintf("<p>%s</p>", kPageText);
    {
      base::AutoLock auto_lock(lock_);
      while (!aborted_.load(std::memory_order_release))
        condition_variable_.Wait();
      terminated_.store(true, std::memory_order_release);
    }
  }
}

const char* InfinitePendingResponseProvider::GetPageText() {
  return "Navigation testing page";
}

GURL InfinitePendingResponseProvider::GetInfinitePendingResponseUrl() const {
  GURL::Replacements replacements;
  replacements.SetPathStr("resource");
  return url_.GetOrigin().ReplaceComponents(replacements);
}
