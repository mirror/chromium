// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_ORIGIN_MANIFEST_ORIGIN_MANIFEST_STORE_IMPL_H_
#define COMPONENTS_ORIGIN_MANIFEST_ORIGIN_MANIFEST_STORE_IMPL_H_

#include <string>

#include "components/origin_manifest/interfaces/origin_manifest.mojom.h"
#include "components/origin_manifest/sqlite_persistent_origin_manifest_store.h"
#include "mojo/public/cpp/bindings/binding_set.h"

namespace origin_manifest {

class OriginManifestStoreFactory;

class OriginManifestStoreImpl : public mojom::OriginManifestStore {
 public:
  OriginManifestStoreImpl(OriginManifestStoreFactory* factory);
  ~OriginManifestStoreImpl() override;

  void SayHello() override;
  void Get(const std::string& origin, GetCallback callback) override;
  void GetCORSPreflight(const std::string& origin,
                        GetCORSPreflightCallback callback) override;

  const char* GetNameForLogging();

 private:
  OriginManifestStoreFactory* factory_;

  DISALLOW_COPY_AND_ASSIGN(OriginManifestStoreImpl);
};

}  // namespace origin_manifest

#endif  // COMPONENTS_ORIGIN_MANIFEST_ORIGIN_MANIFEST_STORE_IMPL_H_
