// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/origin_manifest/origin_manifest_store_impl.h"

#include "base/bind.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "components/origin_manifest/origin_manifest_store_client.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace origin_manifest {

OriginManifestStoreImpl::OriginManifestStoreImpl(
    net::URLRequestContextGetter* getter)
    : OriginManifestStore(getter), getter_(getter) {}

OriginManifestStoreImpl::~OriginManifestStoreImpl() {}

void OriginManifestStoreImpl::BindRequest(
    mojom::OriginManifestStoreRequest request) {
  mojo::MakeStrongBinding(std::make_unique<OriginManifestStoreClient>(this),
                          std::move(request));
}

mojom::OriginManifestPtr OriginManifestStoreImpl::Get(
    const url::Origin& origin) {
  mojom::OriginManifestPtr om(nullptr);

  OriginManifestMap::iterator it = origin_manifest_map.find(origin);
  if (it != origin_manifest_map.end()) {
    om = it->second->Clone();
  }

  return om;
}

void OriginManifestStoreImpl::GetCORSPreflight(
    const url::Origin& origin,
    base::OnceCallback<void(mojom::CORSPreflightPtr)> callback) {
  mojom::CORSPreflightPtr corspreflights;

  mojom::OriginManifestPtr om = Get(origin);
  if (!om.Equals(nullptr))
    corspreflights = om->corspreflights.Clone();

  std::move(callback).Run(std::move(corspreflights));
}

void OriginManifestStoreImpl::GetContentSecurityPolicies(
    const url::Origin& origin,
    base::OnceCallback<void(std::vector<mojom::ContentSecurityPolicyPtr>)>
        callback) {
  std::vector<mojom::ContentSecurityPolicyPtr> csps;

  mojom::OriginManifestPtr om = Get(origin);
  if (!om.Equals(nullptr)) {
    for (auto const& csp : om->csps)
      csps.push_back(csp.Clone());
  }

  std::move(callback).Run(std::move(csps));
}

void OriginManifestStoreImpl::Init(
    SQLitePersistentOriginManifestStore* persistent) {
  persistent_ = persistent;

  if (persistent_) {
    persistent_->Load(base::BindRepeating(
        &OriginManifestStoreImpl::OnInitialLoad, base::Unretained(this)));
  }
}

void OriginManifestStoreImpl::OnInitialLoad(
    std::vector<mojom::OriginManifestPtr> oms) {
  for (std::vector<mojom::OriginManifestPtr>::iterator it = oms.begin();
       it != oms.end(); ++it) {
    GURL url((*it)->origin);
    DCHECK(url.is_valid());

    origin_manifest_map[url::Origin(url)] = std::move(*it);
  }
}

net::URLRequestContextGetter*
OriginManifestStoreImpl::URLRequestContextGetter() {
  DCHECK(getter_);
  return getter_;
}

void OriginManifestStoreImpl::Add(mojom::OriginManifestPtr om,
                                  base::OnceCallback<void()> callback) {
  DCHECK(!om.Equals(nullptr));

  GURL url(om->origin);
  if (!url.is_valid()) {
    std::move(callback).Run();
    return;
  }

  om->origin = url.spec();
  url::Origin origin(url);
  origin_manifest_map[origin] = std::move(om);

  if (persistent_) {
    persistent_->AddOriginManifest(origin_manifest_map[origin]);
  }

  std::move(callback).Run();
}

void OriginManifestStoreImpl::Remove(const url::Origin origin,
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

const char* OriginManifestStoreImpl::GetNameForLogging() {
  return "OriginManifestStoreImpl";
}

}  // namespace origin_manifest
