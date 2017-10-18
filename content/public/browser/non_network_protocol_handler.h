// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_BROWSER_NON_NETWORK_PROTOCOL_HANDLER_H_
#define CONTENT_PUBLIC_BROWSER_NON_NETWORK_PROTOCOL_HANDLER_H_

#include <memory>
#include <string>

#include "content/common/content_export.h"
#include "content/public/common/url_loader.mojom.h"

namespace content {

struct ResourceRequest;

// Base class for handlers of non-network URLs when the Network Service is
// enabled. Always destroyed on the IO thread.
class CONTENT_EXPORT NonNetworkProtocolHandler {
 public:
  virtual ~NonNetworkProtocolHandler() {}

  // Called to initiate loading of a |request|. Always called on the IO thread.
  virtual void CreateAndStartLoader(const ResourceRequest& request,
                                    mojom::URLLoaderRequest loader,
                                    mojom::URLLoaderClientPtr client) = 0;
};

using NonNetworkProtocolHandlerMap =
    std::map<std::string, std::unique_ptr<NonNetworkProtocolHandler>>;

}  // namespace content

#endif  // CONTENT_PUBLIC_BROWSER_NON_NETWORK_PROTOCOL_HANDLER_H_
