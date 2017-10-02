// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/origin_manifest/origin_manifest_store_impl.h"

#include "base/bind.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "content/browser/origin_manifest/origin_manifest_store_client_host.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace content {

OriginManifestStoreImpl::OriginManifestStoreImpl() : OriginManifestStore() {}

OriginManifestStoreImpl::~OriginManifestStoreImpl() {}

void OriginManifestStoreImpl::BindRequestWithRequestContext(
    blink::mojom::OriginManifestStoreRequest request,
    net::URLRequestContextGetter* getter) {
  mojo::MakeStrongBinding(
      std::make_unique<OriginManifestStoreClientHost>(this, getter),
      std::move(request));
}

void OriginManifestStoreImpl::BindRequest(
    blink::mojom::OriginManifestStoreRequest request) {
  mojo::MakeStrongBinding(
      std::make_unique<OriginManifestStoreClientHost>(this, nullptr),
      std::move(request));
}

blink::mojom::OriginManifestPtr OriginManifestStoreImpl::Get(
    const url::Origin& origin) {
  blink::mojom::OriginManifestPtr om(nullptr);

  OriginManifestMap::iterator it = origin_manifest_map.find(origin);
  if (it != origin_manifest_map.end()) {
    om = it->second->Clone();
  }

  return om;
}

void OriginManifestStoreImpl::GetCORSPreflight(
    const url::Origin& origin,
    base::OnceCallback<void(blink::mojom::CORSPreflightPtr)> callback) {
  blink::mojom::CORSPreflightPtr corspreflights;

  blink::mojom::OriginManifestPtr om = Get(origin);
  if (!om.Equals(nullptr))
    corspreflights = om->corspreflights.Clone();

  std::move(callback).Run(std::move(corspreflights));
}

void OriginManifestStoreImpl::GetContentSecurityPolicies(
    const url::Origin& origin,
    base::OnceCallback<
        void(std::vector<blink::mojom::ContentSecurityPolicyPtr>)> callback) {
  std::vector<blink::mojom::ContentSecurityPolicyPtr> csps;

  blink::mojom::OriginManifestPtr om = Get(origin);
  if (!om.Equals(nullptr)) {
    for (auto const& csp : om->csps)
      csps.push_back(csp.Clone());
  }

  std::move(callback).Run(std::move(csps));
}

void OriginManifestStoreImpl::Add(blink::mojom::OriginManifestPtr om,
                                  base::OnceCallback<void()> callback) {
  DCHECK(!om.Equals(nullptr));

  url::Origin origin(om->origin);
  origin_manifest_map[origin] = std::move(om);

  std::move(callback).Run();
}

void OriginManifestStoreImpl::Remove(const url::Origin origin,
                                     base::OnceCallback<void()> callback) {
  blink::mojom::OriginManifestPtr om(nullptr);

  OriginManifestMap::iterator it = origin_manifest_map.find(origin);
  if (it != origin_manifest_map.end()) {
    om = std::move(it->second);
    origin_manifest_map.erase(it);
  }

  std::move(callback).Run();
}

const char* OriginManifestStoreImpl::GetNameForLogging() {
  return "OriginManifestStoreImpl";
}

}  // namespace content
