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

namespace origin_manifest {

OriginManifestStoreImpl::OriginManifestStoreImpl(
    net::URLRequestContextGetter* getter)
    : OriginManifestStore(getter), getter_(getter) {
  /*/ TODO remove test data
    mojom::CORSPreflightPtr cors_preflight = mojom::CORSPreflight::New(
        mojom::CORSNoCredentials::New(), mojom::CORSWithCredentials::New());
    cors_preflight->nocredentials->origins.push_back("*");
    // cors_preflight->withcredentials->origins.push_back("*");

    std::vector<mojom::ContentSecurityPolicyPtr> csps;
    csps.push_back(mojom::ContentSecurityPolicy::New(
        "default-src 'unsafe-inline' 'unsafe-eval' *; frame-src 'none';",
        mojom::Disposition::ENFORCE, true));
    //"default-src 'none';", mojom::CSPMode::ENFORCE, true));

    origin_manifest_map["http://a.com/"] = mojom::OriginManifest::New();
    origin_manifest_map["http://a.com/"]->origin = "http://a.com/";
    origin_manifest_map["http://a.com/"]->version = "manifest1";
    origin_manifest_map["http://a.com/"]->json = "{}";
    origin_manifest_map["http://a.com/"]->corspreflights =
    std::move(cors_preflight); origin_manifest_map["http://a.com/"]->csps =
    std::move(csps);
  */
}

OriginManifestStoreImpl::~OriginManifestStoreImpl() {}

void OriginManifestStoreImpl::BindRequest(
    mojom::OriginManifestStoreRequest request) {
  mojo::MakeStrongBinding(std::make_unique<OriginManifestStoreClient>(this),
                          std::move(request));
}

void OriginManifestStoreImpl::SayHello() {
  VLOG(1) << "OriginManifestStoreImpl says 'Hello!'";
}

mojom::OriginManifestPtr OriginManifestStoreImpl::Get(
    const std::string& origin) {
  mojom::OriginManifestPtr om(nullptr);
  // we have to do this since it might be http://a.com or http://a.com/
  // depending who created the origin string
  GURL url = GURL(origin);
  if (!url.is_valid())
    return om;

  OriginManifestMap::iterator it = origin_manifest_map.find(url.spec());
  if (it != origin_manifest_map.end()) {
    om = it->second->Clone();
  }
  return om;
}

void OriginManifestStoreImpl::GetCORSPreflight(
    const std::string& origin,
    base::OnceCallback<void(mojom::CORSPreflightPtr)> callback) {
  mojom::CORSPreflightPtr corspreflights(nullptr);
  GURL url = GURL(origin);
  if (!url.is_valid()) {
    std::move(callback).Run(std::move(corspreflights));
    return;
  }

  mojom::OriginManifestPtr om = Get(url.spec());
  if (!om.Equals(nullptr)) {
    corspreflights = om->corspreflights.Clone();
  }

  std::move(callback).Run(std::move(corspreflights));
}

void OriginManifestStoreImpl::GetContentSecurityPolicies(
    const std::string& origin,
    base::OnceCallback<void(std::vector<mojom::ContentSecurityPolicyPtr>)>
        callback) {
  std::vector<mojom::ContentSecurityPolicyPtr> csps;
  GURL url = GURL(origin);
  if (!url.is_valid()) {
    std::move(callback).Run(std::move(csps));
    return;
  }

  mojom::OriginManifestPtr om = Get(url.spec());
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
    // We assume here that the database gives us only good data
    DCHECK(GURL((*it)->origin).is_valid());
    origin_manifest_map[(*it)->origin] = std::move(*it);
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

  GURL url = GURL(om->origin);
  if (!url.is_valid()) {
    std::move(callback).Run();
    return;
  }

  om->origin = url.spec();
  std::string origin = om->origin;
  origin_manifest_map[origin] = std::move(om);

  if (persistent_) {
    persistent_->AddOriginManifest(origin_manifest_map[origin]);
  }

  std::move(callback).Run();
}

void OriginManifestStoreImpl::Remove(const std::string origin,
                                     base::OnceCallback<void()> callback) {
  mojom::OriginManifestPtr om(nullptr);
  GURL url = GURL(origin);
  if (!url.is_valid()) {
    std::move(callback).Run();
    return;
  }

  OriginManifestMap::iterator it = origin_manifest_map.find(url.spec());
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
