// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_NETWORK_NETWORK_SERVICE_H_
#define CONTENT_PUBLIC_NETWORK_NETWORK_SERVICE_H_

#include <memory>

#include "content/common/content_export.h"
#include "content/public/common/network_service.mojom.h"

namespace net {
class NetLog;
class URLRequestContext;
}  // namespace net

namespace content {

// Allows an in-process NetworkService to be set up.
class CONTENT_EXPORT NetworkService : public mojom::NetworkService {
 public:
  // Creates a NetworkService instance on the current thread, optionally using
  // the passed-in NetLog. Does not take ownership of |net_log|. Must be
  // destroyed before |net_log|.
  //
  // TODO(https://crbug.com/767450): Make it so NetworkService can always create
  // its own NetLog, instead of sharing one.
  static std::unique_ptr<NetworkService> Create(
      mojom::NetworkServiceRequest request,
      net::NetLog* net_log = nullptr);

  // Can be used to seed a NetworkContext with a consumer-configured
  // URLRequestContext. The returned NetworkContext must be destroyed before the
  // NetworkService and the given URLRequestContext.
  //
  // This method is intended to ease the transition to an out-of-process
  // NetworkService, and will be removed once that ships.
  virtual std::unique_ptr<mojom::NetworkContext> CreateNetworkContextImpl(
      mojom::NetworkContextRequest request,
      net::URLRequestContext* url_request_context) = 0;

  ~NetworkService() override {}

 protected:
  NetworkService() {}
};

}  // namespace content

#endif  // CONTENT_PUBLIC_NETWORK_NETWORK_SERVICE_H_
