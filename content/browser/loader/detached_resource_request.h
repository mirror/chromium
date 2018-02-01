// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_LOADER_DETACHED_RESOURCE_REQUEST_H
#define CONTENT_BROWSER_LOADER_DETACHED_RESOURCE_REQUEST_H

#include <memory>

#include "base/memory/scoped_refptr.h"
#include "content/common/content_export.h"
#include "url/gurl.h"

namespace net {
class URLRequestContextGetter;
}

namespace content {

class BrowserContext;
class ResourceContext;
class ResourceRequesterInfo;
class ResourceDispatcherHostImpl;

// Detached resource request, that is a subresource request initiated from the
// browser process, and which starts as detached from any client.
//
// It is intended to provide "detached" request capabilities from the browser
// process, that is like <a ping> or <link rel="prefetch">.
//
// This is a UI thread class.
class CONTENT_EXPORT DetachedResourceRequest {
 public:
  ~DetachedResourceRequest();

  // Creates a detached request to a |url|, with a given initiating URL,
  // |first_party_for_cookies|.
  static std::unique_ptr<DetachedResourceRequest> Create(
      BrowserContext* browser_context,
      const GURL& url,
      const GURL& first_party_for_cookies);

  // Starts a |request|, taking ownership of it.
  static void Start(std::unique_ptr<DetachedResourceRequest> request);

 private:
  DetachedResourceRequest(
      ResourceDispatcherHostImpl* rdh,
      net::URLRequestContextGetter* url_request_context_getter,
      ResourceContext* resource_context,
      const GURL& url,
      const GURL& site_for_cookies);

  void StartOnIOThread();

  ResourceDispatcherHostImpl* rdh_;
  net::URLRequestContextGetter* url_request_context_getter_;
  ResourceContext* resource_context_;
  const GURL url_;
  const GURL site_for_cookies_;
  scoped_refptr<ResourceRequesterInfo> requester_info_;

  DISALLOW_COPY_AND_ASSIGN(DetachedResourceRequest);
};

}  // namespace content

#endif  // CONTENT_BROWSER_LOADER_DETACHED_RESOURCE_REQUEST_H
