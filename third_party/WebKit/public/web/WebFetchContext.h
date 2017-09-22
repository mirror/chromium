// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebFetchContext_h
#define WebFetchContext_h

#include "public/platform/WebCommon.h"
#include "public/web/WebAssociatedURLLoader.h"

namespace blink {

class WebSecurityOrigin;

class BLINK_PLATFORM_EXPORT WebFetchContext {
 public:
  virtual ~WebFetchContext() {}

  virtual WebSecurityOrigin GetSecurityOrigin() = 0;
  virtual std::unique_ptr<WebAssociatedURLLoader> CreateUrlLoader(
      const WebAssociatedURLLoaderOptions&) = 0;
};

}  // namespace blink

#endif  // WebFetchContext_h
