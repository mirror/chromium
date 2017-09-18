// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_ORIGIN_MANIFEST_ORIGIN_MANIFEST_STORE_CLIENT_H_
#define COMPONENTS_ORIGIN_MANIFEST_ORIGIN_MANIFEST_STORE_CLIENT_H_

#include <map>
#include <string>

#include "components/origin_manifest/interfaces/origin_manifest.mojom.h"
#include "components/origin_manifest/sqlite_persistent_origin_manifest_store.h"
#include "mojo/public/cpp/bindings/binding_set.h"

namespace origin_manifest {

class OriginManifestStoreImpl;
class SQLitePersistentOriginManifestStore;

class OriginManifestStoreClient : public mojom::OriginManifestStore {
 public:
  OriginManifestStoreClient(OriginManifestStoreImpl* store);
  ~OriginManifestStoreClient() override;

  // mojom::OriginManfiestStore implementation.
  void SayHello() override;

  void Get(const std::string& origin, GetCallback callback) override;

  void GetOrFetch(const std::string& origin,
                  const std::string& version,
                  GetCallback callback) override;
  void OnFetchComplete(GetCallback callback, mojom::OriginManifestPtr om);

  void GetCORSPreflight(const std::string& origin,
                        GetCORSPreflightCallback callback) override;
  void GetContentSecurityPolicies(
      const std::string& origin,
      GetContentSecurityPoliciesCallback callback) override;

  const char* GetNameForLogging();

 private:
  // the client does not own the store object!
  OriginManifestStoreImpl* origin_manifest_store_impl_;

  DISALLOW_COPY_AND_ASSIGN(OriginManifestStoreClient);
};

}  // namespace origin_manifest

#endif  // COMPONENTS_ORIGIN_MANIFEST_ORIGIN_MANIFEST_STORE_CLIENT_H_
