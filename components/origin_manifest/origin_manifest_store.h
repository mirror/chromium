// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_ORIGIN_MANIFEST_ORIGIN_MANIFEST_STORE_H_
#define COMPONENTS_ORIGIN_MANIFEST_ORIGIN_MANIFEST_STORE_H_

#include "components/origin_manifest/interfaces/origin_manifest.mojom.h"

namespace net {
class URLRequestContextGetter;
}

namespace origin_manifest {

class SQLitePersistentOriginManifestStore;

class OriginManifestStore {
 public:
  OriginManifestStore(net::URLRequestContextGetter* getter){};
  virtual ~OriginManifestStore(){};

  virtual void BindRequest(mojom::OriginManifestStoreRequest request) = 0;
  virtual void Init(SQLitePersistentOriginManifestStore* persistent) = 0;
};

}  // namespace origin_manifest

#endif  // COMPONENTS_ORIGIN_MANIFEST_ORIGIN_MANIFEST_STORE_H_
