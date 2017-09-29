// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_ORIGIN_MANIFEST_ORIGIN_MANIFEST_STORE_CLIENT_HOST_H_
#define CONTENT_BROWSER_ORIGIN_MANIFEST_ORIGIN_MANIFEST_STORE_CLIENT_HOST_H_

#include <map>
#include <string>

#include "content/public/common/origin_manifest.mojom.h"
#include "mojo/public/cpp/bindings/binding_set.h"

namespace net {
class URLRequestContextGetter;
}  // namespace net

namespace content {

class OriginManifestStoreImpl;

class OriginManifestStoreClientHost : public mojom::OriginManifestStore {
 public:
  OriginManifestStoreClientHost(OriginManifestStoreImpl* store,
                                net::URLRequestContextGetter* getter);
  ~OriginManifestStoreClientHost() override;

  // mojom::OriginManfiestStore implementation: async
  void Get(const std::string& origin, GetCallback callback) override;
  void GetOrFetch(const std::string& origin,
                  const std::string& version,
                  GetCallback callback) override;
  void Remove(const std::string& origin, RemoveCallback callback) override;

  // mojom::OriginManfiestStore implementation: sync
  void GetCORSPreflight(const std::string& origin,
                        GetCORSPreflightCallback callback) override;
  void GetContentSecurityPolicies(
      const std::string& origin,
      GetContentSecurityPoliciesCallback callback) override;

  void OnFetchComplete(GetCallback callback, mojom::OriginManifestPtr om);

  const char* GetNameForLogging();

 private:
  // the client does not own the store object!
  OriginManifestStoreImpl* origin_manifest_store_impl_;
  net::URLRequestContextGetter* getter_;

  DISALLOW_COPY_AND_ASSIGN(OriginManifestStoreClientHost);
};

}  // namespace content

#endif  // CONTENT_BROWSER_ORIGIN_MANIFEST_ORIGIN_MANIFEST_STORE_CLIENT_HOST_H_
