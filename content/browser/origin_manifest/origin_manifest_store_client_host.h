// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_ORIGIN_MANIFEST_ORIGIN_MANIFEST_STORE_CLIENT_HOST_H_
#define CONTENT_BROWSER_ORIGIN_MANIFEST_ORIGIN_MANIFEST_STORE_CLIENT_HOST_H_

#include <map>
#include <string>

#include "mojo/public/cpp/bindings/binding_set.h"
#include "third_party/WebKit/public/platform/origin_manifest.mojom.h"
#include "third_party/WebKit/public/platform/origin_manifest_store.mojom.h"

namespace net {
class URLRequestContextGetter;
}  // namespace net

namespace url {
class Origin;
}  // namespace url

namespace content {

class OriginManifestStoreImpl;

class OriginManifestStoreClientHost : public blink::mojom::OriginManifestStore {
 public:
  OriginManifestStoreClientHost(OriginManifestStoreImpl* store,
                                net::URLRequestContextGetter* getter);
  ~OriginManifestStoreClientHost() override;

  // blink::mojom::OriginManfiestStore implementation: async
  void Get(const url::Origin& origin, GetCallback callback) override;
  void GetOrFetch(const url::Origin& origin,
                  const std::string& version,
                  GetCallback callback) override;
  void Remove(const url::Origin& origin, RemoveCallback callback) override;

  // blink::mojom::OriginManfiestStore implementation: sync
  void GetCORSPreflight(const url::Origin& origin,
                        GetCORSPreflightCallback callback) override;
  void GetContentSecurityPolicies(
      const url::Origin& origin,
      GetContentSecurityPoliciesCallback callback) override;

  void OnFetchComplete(GetCallback callback,
                       blink::mojom::OriginManifestPtr om);

  const char* GetNameForLogging();

 private:
  // the client does not own the store object!
  OriginManifestStoreImpl* origin_manifest_store_impl_;
  net::URLRequestContextGetter* getter_;

  DISALLOW_COPY_AND_ASSIGN(OriginManifestStoreClientHost);
};

}  // namespace content

#endif  // CONTENT_BROWSER_ORIGIN_MANIFEST_ORIGIN_MANIFEST_STORE_CLIENT_HOST_H_
