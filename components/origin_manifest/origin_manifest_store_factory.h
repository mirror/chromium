// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_ORIGIN_MANIFEST_ORIGIN_MANIFEST_STORE_FACTORY_H_
#define COMPONENTS_ORIGIN_MANIFEST_ORIGIN_MANIFEST_STORE_FACTORY_H_

#include <map>
#include <string>

#include "components/origin_manifest/interfaces/origin_manifest.mojom.h"

namespace origin_manifest {

class SQLitePersistentOriginManifestStore;

class OriginManifestStoreFactory {
 public:
  OriginManifestStoreFactory();
  ~OriginManifestStoreFactory();

  static void BindRequest(mojom::OriginManifestStoreRequest request);

  void Init(SQLitePersistentOriginManifestStore* persistent);

  void Add(mojom::OriginManifestPtr om, base::OnceCallback<void()> callback);
  void Get(const std::string origin,
           base::OnceCallback<void(mojom::OriginManifestPtr)> callback);
  void Remove(const std::string origin, base::OnceCallback<void()> callback);

  const char* GetNameForLogging();

 private:
  typedef std::map<std::string, mojom::OriginManifestPtr> OriginManifestMap;

  void OnInitialLoad(std::vector<mojom::OriginManifestPtr> oms);

  OriginManifestMap origin_manifest_map;
  scoped_refptr<SQLitePersistentOriginManifestStore> persistent_;

  DISALLOW_COPY_AND_ASSIGN(OriginManifestStoreFactory);
};

}  // namespace origin_manifest

#endif  // COMPONENTS_ORIGIN_MANIFEST_ORIGIN_MANIFEST_STORE_FACTORY_H_
