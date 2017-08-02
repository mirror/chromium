// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_TEST_URL_REQUEST_URL_REQUEST_HANGING_GET_JOB_H_
#define NET_TEST_URL_REQUEST_URL_REQUEST_HANGING_GET_JOB_H_

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "net/url_request/url_request_job.h"

namespace net {

class URLRequest;

// A URLRequestJob that hangs when try to read response body.
class URLRequestHangingGetJob : public URLRequestJob {
 public:
  URLRequestHangingGetJob(URLRequest* request,
                          NetworkDelegate* network_delegate);

  void Start() override;

  // Adds the testing URLs to the URLRequestFilter.
  static void AddUrlHandler();

  static GURL GetMockHttpUrl();
  static GURL GetMockHttpsUrl();

 private:
  void GetResponseInfoConst(HttpResponseInfo* info) const;
  ~URLRequestHangingGetJob() override;

  base::WeakPtrFactory<URLRequestHangingGetJob> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(URLRequestHangingGetJob);
};

}  // namespace net

#endif  // NET_TEST_URL_REQUEST_URL_REQUEST_HANGING_GET_JOB_H_
