// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef OriginManifestStoreClient_h
#define OriginManifestStoreClient_h

#include "components/origin_manifest/interfaces/origin_manifest.mojom-blink.h"
#include "platform/PlatformExport.h"
#include "platform/network/ContentSecurityPolicyParsers.h"

namespace blink {

class ContentSecurityPolicy;
class KURL;
class ResourceRequest;
class SecurityOrigin;

class PLATFORM_EXPORT OriginManifestStoreClient {
 public:
  OriginManifestStoreClient();
  virtual ~OriginManifestStoreClient();

  // CORS related functions
  bool DefinesCORSPreflight(ResourceRequest&);
  void SetCORSFetchModes(ResourceRequest&);

  // CSP related functions
  bool DefinesContentSecurityPolicies(const SecurityOrigin*);
  WTF::Vector<WTF::String> GetContentSecurityPolicies(
      const SecurityOrigin*,
      ContentSecurityPolicyHeaderType,
      bool);

 private:
  origin_manifest::mojom::blink::OriginManifestStorePtr store_;

  origin_manifest::mojom::blink::CORSPreflightPtr cors_preflight_;
  WTF::Vector<origin_manifest::mojom::blink::ContentSecurityPolicyPtr> csps_;
};

}  // namespace blink

#endif
