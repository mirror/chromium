// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_ORIGIN_MANIFEST_STORE_H_
#define CONTENT_PUBLIC_ORIGIN_MANIFEST_STORE_H_

#include "third_party/WebKit/public/platform/origin_manifest_store.mojom.h"

namespace net {
class URLRequestContextGetter;
}  // namespace net

namespace content {

class OriginManifestStore {
 public:
  OriginManifestStore(){};
  virtual ~OriginManifestStore(){};

  virtual void BindRequestWithRequestContext(
      blink::mojom::OriginManifestStoreRequest request,
      net::URLRequestContextGetter* getter) = 0;
  virtual void BindRequest(
      blink::mojom::OriginManifestStoreRequest request) = 0;
};

}  // namespace content

#endif  // CONTENT_PUBLIC_ORIGIN_MANIFEST_STORE_H_
