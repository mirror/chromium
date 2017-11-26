// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_FRAME_HOST_CROSS_SITE_DOCUMENT_BLOCKING_THROTTLE_H_
#define CONTENT_BROWSER_FRAME_HOST_CROSS_SITE_DOCUMENT_BLOCKING_THROTTLE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "content/public/browser/resource_throttle.h"
#include "content/public/common/resource_type.h"
#include "net/http/http_response_headers.h"
#include "url/gurl.h"

namespace net {
class URLRequest;
}  // namespace net

namespace content {

// A ResourceThrottle that prevents the renderer process from receiving network
// responses that contain cross-site documents, with appropriate exceptions to
// preserve compatibility.
class CONTENT_EXPORT CrossSiteDocumentBlockingThrottle
    : public ResourceThrottle {
 public:
  // Instantiates a throttle for the given if it's supported for the given
  // |request|. The caller must guarantee that |request| outlives the throttle.
  static std::unique_ptr<ResourceThrottle> MaybeCreateThrottleForRequest(
      net::URLRequest* request,
      ResourceType resource_type);

  ~CrossSiteDocumentBlockingThrottle() override;

  // ResourceThrottle implementation:
  const char* GetNameForLogging() const override;
  void WillProcessResponse(bool* defer) override;

 private:
  CrossSiteDocumentBlockingThrottle(net::URLRequest* request,
                                    ResourceType resource_type);

  // The request this throttle is observing.
  net::URLRequest* request_;

  // The type of the request, as claimed by the renderer process.
  ResourceType resource_type_;

  DISALLOW_COPY_AND_ASSIGN(CrossSiteDocumentBlockingThrottle);
};

}  // namespace content

#endif  // CONTENT_BROWSER_FRAME_HOST_CROSS_SITE_DOCUMENT_BLOCKING_THROTTLE_H_
