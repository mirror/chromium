// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_NETWORK_NETWORK_CONTEXT_H_
#define CONTENT_PUBLIC_NETWORK_NETWORK_CONTEXT_H_

#include <memory>

#include "content/common/content_export.h"
#include "content/public/common/network_service.mojom.h"

namespace net {
class URLRequestContext;
}  // namespace net

namespace content {

// See mojom::NetworkContext and NetworkContextImpl for details on this class.
class CONTENT_EXPORT NetworkContext : public mojom::NetworkContext {
 public:
  // Creates a NetworkContext that can be used to provide access to
  // |url_request_context|, which must outlive the NetworkContext
  //
  // This method is intended to ease the transition to using a network service,
  // and will be removed once that ships.
  static std::unique_ptr<NetworkContext> CreateForURLRequestContext(
      mojom::NetworkContextRequest network_context_request,
      net::URLRequestContext* url_request_context);

  ~NetworkContext() override {}

 protected:
  NetworkContext() {}
};

}  // namespace content

#endif  // CONTENT_PUBLIC_NETWORK_NETWORK_CONTEXT_H_
