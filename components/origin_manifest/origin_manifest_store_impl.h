// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_ORIGIN_MANIFEST_ORIGIN_MANIFEST_STORE_IMPL_H_
#define COMPONENTS_ORIGIN_MANIFEST_ORIGIN_MANIFEST_STORE_IMPL_H_

#include <map>
#include <string>

#include "components/origin_manifest/interfaces/origin_manifest.mojom.h"
#include "components/origin_manifest/sqlite_persistent_origin_manifest_store.h"
#include "mojo/public/cpp/bindings/binding_set.h"

namespace net {
class URLRequestContextGetter;
}

namespace origin_manifest {

class SQLitePersistentOriginManifestStore;

class OriginManifestStoreImpl {
 public:
  OriginManifestStoreImpl(net::URLRequestContextGetter* getter);
  virtual ~OriginManifestStoreImpl();

  // static stuff
  static void BindRequest(mojom::OriginManifestStoreRequest request);

  // mojom::OriginManfiestStore implementation.
  void SayHello();
  mojom::OriginManifestPtr Get(const std::string& origin);
  void GetCORSPreflight(
      const std::string& origin,
      base::OnceCallback<void(mojom::CORSPreflightPtr)> callback);
  void GetContentSecurityPolicies(
      const std::string& origin,
      base::OnceCallback<void(std::vector<mojom::ContentSecurityPolicyPtr>)>
          callback);

  void Init(SQLitePersistentOriginManifestStore* persistent);
  void OnInitialLoad(std::vector<mojom::OriginManifestPtr> oms);

  net::URLRequestContextGetter* URLRequestContextGetter();
  void Add(mojom::OriginManifestPtr om, base::OnceCallback<void()> callback);
  void Remove(const std::string origin, base::OnceCallback<void()> callback);

  const char* GetNameForLogging();

 private:
  typedef std::map<std::string, mojom::OriginManifestPtr> OriginManifestMap;

  net::URLRequestContextGetter* getter_;
  OriginManifestMap origin_manifest_map;
  scoped_refptr<SQLitePersistentOriginManifestStore> persistent_;

  DISALLOW_COPY_AND_ASSIGN(OriginManifestStoreImpl);
};

}  // namespace origin_manifest

#endif  // COMPONENTS_ORIGIN_MANIFEST_ORIGIN_MANIFEST_STORE_IMPL_H_
