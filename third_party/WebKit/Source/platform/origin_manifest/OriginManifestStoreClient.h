// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef OriginManifestStoreClient_h
#define OriginManifestStoreClient_h

#include "components/origin_manifest/interfaces/origin_manifest.mojom-blink.h"
#include "platform/PlatformExport.h"

namespace blink {

class ResourceRequest;
class SecurityOrigin;

class PLATFORM_EXPORT OriginManifestStoreClient {
 public:
  OriginManifestStoreClient();
  virtual ~OriginManifestStoreClient();

  bool DefinesCORSPreflight(SecurityOrigin*);
  void SetCORSFetchModes(ResourceRequest&);

 private:
  origin_manifest::mojom::blink::OriginManifestStorePtr store_;
  origin_manifest::mojom::blink::CORSPreflightPtr cors_preflight_;
};

}  // namespace blink

#endif
