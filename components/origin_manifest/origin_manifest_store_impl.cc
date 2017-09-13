// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/origin_manifest/origin_manifest_store_impl.h"

#include "base/bind.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "components/origin_manifest/origin_manifest_store_factory.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

namespace origin_manifest {

OriginManifestStoreImpl::OriginManifestStoreImpl(
    OriginManifestStoreFactory* factory)
    : factory_(factory) {}

OriginManifestStoreImpl::~OriginManifestStoreImpl() {}

void OriginManifestStoreImpl::SayHello() {
  VLOG(1) << "OriginManifestStoreImpl says 'Hello!'";
}

void OriginManifestStoreImpl::Get(const std::string& origin,
                                  GetCallback callback) {
  factory_->Get(origin, std::move(callback));
}

void OriginManifestStoreImpl::GetCORSPreflight(
    const std::string& origin,
    GetCORSPreflightCallback callback) {
  VLOG(1) << "And everybody is like 'Woop woop' (" << origin << ")";

  mojom::CORSPreflightPtr cors_preflight = mojom::CORSPreflight::New(
      mojom::CORSNoCredentials::New(), mojom::CORSWithCredentials::New());
  cors_preflight->nocredentials->origins.push_back("*");
  // cors_preflight->withcredentials->origins.push_back("*");

  std::move(callback).Run(std::move(cors_preflight));
}

/*
std::unique_ptr<OriginManifestStoreClient> OriginManifestStoreImpl::GetStore() {
  return base::MakeUnique<OriginManifestStoreClient>(store.Pointer());
}

void OriginManifestStoreImpl::Create(SQLitePersistentOriginManifestStore*
persistent) { persistent_ = persistent;

  if (persistent_) {
    persistent_->Load(base::BindRepeating(&OriginManifestStoreImpl::OnInitialLoad,
base::Unretained(this)));
  }
}

void
OriginManifestStoreImpl::OnInitialLoad(std::vector<mojom::OriginManifestPtr>
oms) { for (std::vector<mojom::OriginManifestPtr>::iterator it = oms.begin(); it
!= oms.end(); ++it) { origin_manifest_map[(*it)->origin] = std::move(*it);
  }
}

void OriginManifestStoreImpl::BindRequest(mojom::OriginManifestStoreRequest
request) { bindings_.AddBinding(this, std::move(request));
}

void OriginManifestStoreImpl::Add(mojom::OriginManifestPtr om, AddCallback
callback) { DCHECK(!om.Equals(nullptr));

  origin_manifest_map[om->origin] = std::move(om);

  if (persistent_) {
    persistent_->AddOriginManifest(om);
  }

  std::move(callback).Run();
}

void OriginManifestStoreImpl::Get(const std::string& origin, GetCallback
callback) { mojom::OriginManifestPtr om(nullptr); OriginManifestMap::iterator it
= origin_manifest_map.find(origin); if (it != origin_manifest_map.end()) { om =
it->second->Clone();
  }
  std::move(callback).Run(std::move(om));
}

void OriginManifestStoreImpl::Remove(const std::string& origin, RemoveCallback
callback) { mojom::OriginManifestPtr om(nullptr); OriginManifestMap::iterator it
= origin_manifest_map.find(origin); if (it != origin_manifest_map.end()) { om =
std::move(it->second); origin_manifest_map.erase(it); if (persistent_) {
      persistent_->DeleteOriginManifest(om);
    }
  }

  std::move(callback).Run();
}
*/

const char* OriginManifestStoreImpl::GetNameForLogging() {
  return "OriginManifestStoreImpl";
}

}  // namespace origin_manifest
