// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebPresentationConnection_h
#define WebPresentationConnection_h

#include "public/platform/WebCommon.h"

namespace blink {

// Implemented by the embedder for a PresentationConnection.
class WebPresentationConnection {
 public:
  virtual ~WebPresentationConnection() = default;

  // Initializes a controller PresentationConnection. It is an error to call
  // this for a receiver PresentationConnection.
  // Note: This is a bit hacky considering this interface is also used
  // for receiver PresentationConnections, but this interface will be removed
  // altogether once crbug.com/749327 is fixed.
  virtual void InitForController() = 0;
};

}  // namespace blink

#endif  // WebPresentationConnection_h
