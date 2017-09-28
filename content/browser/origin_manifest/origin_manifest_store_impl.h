// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_ORIGIN_MANIFEST_ORIGIN_MANIFEST_STORE_IMPL_H_
#define CONTENT_BROWSER_ORIGIN_MANIFEST_ORIGIN_MANIFEST_STORE_IMPL_H_

#include <map>

#include "content/common/content_export.h"
#include "content/public/browser/origin_manifest_store.h"
#include "content/public/common/origin_manifest.mojom.h"
#include "mojo/public/cpp/bindings/binding_set.h"

namespace url {
class Origin;
}  // namespace url

namespace content {

class CONTENT_EXPORT OriginManifestStoreImpl : public OriginManifestStore {
 public:
  OriginManifestStoreImpl();
  ~OriginManifestStoreImpl() override;

  // OriginManifestStore implementation
  void BindRequest(mojom::OriginManifestStoreRequest request,
                   net::URLRequestContextGetter* getter) override;

  // mojom::OriginManfiestStore implementation.
  mojom::OriginManifestPtr Get(const url::Origin& origin);
  void GetCORSPreflight(
      const url::Origin& origin,
      base::OnceCallback<void(mojom::CORSPreflightPtr)> callback);
  void GetContentSecurityPolicies(
      const url::Origin& origin,
      base::OnceCallback<void(std::vector<mojom::ContentSecurityPolicyPtr>)>
          callback);

  void OnInitialLoad(std::vector<mojom::OriginManifestPtr> oms);

  void Add(mojom::OriginManifestPtr om, base::OnceCallback<void()> callback);
  void Remove(const url::Origin origin, base::OnceCallback<void()> callback);

  const char* GetNameForLogging();

 private:
  typedef std::map<url::Origin, mojom::OriginManifestPtr> OriginManifestMap;

  OriginManifestMap origin_manifest_map;

  DISALLOW_COPY_AND_ASSIGN(OriginManifestStoreImpl);
};

}  // namespace content

#endif  // CONTENT_BROWSER_ORIGIN_MANIFEST_ORIGIN_MANIFEST_STORE_IMPL_H_
