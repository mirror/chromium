// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/origin_manifest/origin_manifest_store_factory.h"

#include "base/bind.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "components/origin_manifest/origin_manifest_store_impl.h"
#include "components/origin_manifest/sqlite_persistent_origin_manifest_store.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

namespace origin_manifest {

OriginManifestStoreFactory::OriginManifestStoreFactory()
    : persistent_(nullptr) {}

OriginManifestStoreFactory::~OriginManifestStoreFactory() {}

// static
void OriginManifestStoreFactory::BindRequest(
    mojom::OriginManifestStoreRequest request) {
  mojo::MakeStrongBinding(std::make_unique<OriginManifestStoreImpl>(
                              new OriginManifestStoreFactory()),
                          std::move(request));
}

void OriginManifestStoreFactory::Init(
    SQLitePersistentOriginManifestStore* persistent) {
  persistent_ = persistent;

  if (persistent_) {
    persistent_->Load(base::BindRepeating(
        &OriginManifestStoreFactory::OnInitialLoad, base::Unretained(this)));
  }
}

void OriginManifestStoreFactory::Add(mojom::OriginManifestPtr om,
                                     base::OnceCallback<void()> callback) {
  DCHECK(!om.Equals(nullptr));

  origin_manifest_map[om->origin] = std::move(om);

  if (persistent_) {
    persistent_->AddOriginManifest(om);
  }

  std::move(callback).Run();
}

void OriginManifestStoreFactory::Get(
    const std::string origin,
    base::OnceCallback<void(mojom::OriginManifestPtr)> callback) {
  VLOG(1) << "origin: " << origin;
  mojom::OriginManifestPtr om(nullptr);
  OriginManifestMap::iterator it = origin_manifest_map.find(origin);
  if (it != origin_manifest_map.end()) {
    om = it->second->Clone();
  }
  std::move(callback).Run(std::move(om));

  /*
  mojom::OriginManifestPtr om(nullptr);
  std::move(callback).Run(std::move(om));
  */
}

void OriginManifestStoreFactory::Remove(const std::string origin,
                                        base::OnceCallback<void()> callback) {
  mojom::OriginManifestPtr om(nullptr);
  OriginManifestMap::iterator it = origin_manifest_map.find(origin);
  if (it != origin_manifest_map.end()) {
    om = std::move(it->second);
    origin_manifest_map.erase(it);
    if (persistent_) {
      persistent_->DeleteOriginManifest(om);
    }
  }

  std::move(callback).Run();
}

void OriginManifestStoreFactory::OnInitialLoad(
    std::vector<mojom::OriginManifestPtr> oms) {
  for (std::vector<mojom::OriginManifestPtr>::iterator it = oms.begin();
       it != oms.end(); ++it) {
    origin_manifest_map[(*it)->origin] = std::move(*it);
  }
}

/*
void OriginManifestStoreFactory::Get(const std::string& origin, GetCallback
callback) { mojom::OriginManifestPtr om(nullptr); OriginManifestMap::iterator it
= origin_manifest_map.find(origin); if (it != origin_manifest_map.end()) { om =
it->second->Clone();
  }
  std::move(callback).Run(std::move(om));
}

*/

const char* OriginManifestStoreFactory::GetNameForLogging() {
  return "OriginManifestStoreFactory";
}

}  // namespace origin_manifest
