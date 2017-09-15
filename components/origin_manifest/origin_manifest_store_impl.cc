// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/origin_manifest/origin_manifest_store_impl.h"

#include "base/bind.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "components/origin_manifest/origin_manifest_store_client.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

namespace {
origin_manifest::OriginManifestStoreImpl* static_store_ptr = nullptr;
}

namespace origin_manifest {

OriginManifestStoreImpl::OriginManifestStoreImpl(
    net::URLRequestContextGetter* getter)
    : getter_(getter) {
  static_store_ptr = this;
}

OriginManifestStoreImpl::~OriginManifestStoreImpl() {}

// static
void OriginManifestStoreImpl::BindRequest(
    mojom::OriginManifestStoreRequest request) {
  mojo::MakeStrongBinding(
      std::make_unique<OriginManifestStoreClient>(static_store_ptr),
      std::move(request));
}

void OriginManifestStoreImpl::SayHello() {
  VLOG(1) << "OriginManifestStoreImpl says 'Hello!'";
}

mojom::OriginManifestPtr OriginManifestStoreImpl::Get(
    const std::string& origin) {
  VLOG(1) << "origin: " << origin;
  mojom::OriginManifestPtr om(nullptr);
  OriginManifestMap::iterator it = origin_manifest_map.find(origin);
  if (it != origin_manifest_map.end()) {
    om = it->second->Clone();
  }
  return om;
}

void OriginManifestStoreImpl::GetCORSPreflight(
    const std::string& origin,
    base::OnceCallback<void(mojom::CORSPreflightPtr)> callback) {
  VLOG(1) << "And everybody is like 'Woop woop' (" << origin << ")";

  mojom::CORSPreflightPtr cors_preflight = mojom::CORSPreflight::New(
      mojom::CORSNoCredentials::New(), mojom::CORSWithCredentials::New());
  cors_preflight->nocredentials->origins.push_back("*");
  // cors_preflight->withcredentials->origins.push_back("*");

  std::move(callback).Run(std::move(cors_preflight));
}

void OriginManifestStoreImpl::GetContentSecurityPolicies(
    const std::string& origin,
    base::OnceCallback<void(std::vector<mojom::ContentSecurityPolicyPtr>)>
        callback) {
  std::vector<mojom::ContentSecurityPolicyPtr> csps;
  csps.push_back(mojom::ContentSecurityPolicy::New(
      "default-src 'unsafe-inline' 'unsafe-eval' *; frame-src 'none';",
      mojom::CSPMode::ENFORCE, true));
  //"default-src 'none';", mojom::CSPMode::ENFORCE, true));
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

  origin_manifest_map[om->origin] = std::move(om);

  if (persistent_) {
    persistent_->AddOriginManifest(om);
  }

  std::move(callback).Run();
}

void OriginManifestStoreImpl::Remove(const std::string origin,
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
