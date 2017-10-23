// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_BROWSER_PROTOCOL_HANDLER_H_
#define CONTENT_PUBLIC_BROWSER_PROTOCOL_HANDLER_H_

#include "content/common/content_export.h"
#include "content/public/common/url_loader.mojom.h"

namespace content {

struct ResourceRequest;

// Base class for handlers of non-network URLs when the Network Service is
// enabled.
class CONTENT_EXPORT ProtocolHandler {
 public:
  virtual ~ProtocolHandler() {}

  // Called to initiate loading of a |request|.
  virtual void CreateAndStartLoader(const ResourceRequest& request,
                                    mojom::URLLoaderRequest loader,
                                    mojom::URLLoaderClientPtr client) = 0;
};

}  // namespace content

#endif  // CONTENT_PUBLIC_BROWSER_PROTOCOL_HANDLER_H_
