// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/origin_manifest/origin_manifest_store_client.h"

#include "base/bind.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "components/origin_manifest/origin_manifest_fetcher.h"
#include "components/origin_manifest/origin_manifest_store_impl.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

namespace origin_manifest {

OriginManifestStoreClient::OriginManifestStoreClient(
    OriginManifestStoreImpl* store)
    : origin_manifest_store_impl_(store) {}

OriginManifestStoreClient::~OriginManifestStoreClient() {}

void OriginManifestStoreClient::SayHello() {
  origin_manifest_store_impl_->SayHello();
}

void OriginManifestStoreClient::Get(const std::string& origin,
                                    GetCallback callback) {
  std::move(callback).Run(origin_manifest_store_impl_->Get(origin));
}

void OriginManifestStoreClient::GetOrFetch(const std::string& origin,
                                           const std::string& version,
                                           GetCallback callback) {
  mojom::OriginManifestPtr om = origin_manifest_store_impl_->Get(origin);

  if (om.Equals(nullptr) || (om->version != version)) {
    // TODO(dhausknecht): I am not so sure about this dangling fetcher. It
    // cleans up itself on OnFetchComplete. But if the client gets destroyed
    // while it is not complete... and the client is managed by the
    // StringBinding.
    OriginManifestFetcher* fetcher = new OriginManifestFetcher(
        origin, version,
        origin_manifest_store_impl_->URLRequestContextGetter());
    fetcher->Fetch(base::BindOnce(&OriginManifestStoreClient::OnFetchComplete,
                                  base::Unretained(this), std::move(callback)));
    return;
  }

  std::move(callback).Run(std::move(om));
}

void OriginManifestStoreClient::OnFetchComplete(GetCallback callback,
                                                mojom::OriginManifestPtr om) {
  if (om.Equals(nullptr)) {
    std::move(callback).Run(std::move(om));
    return;
  }

  origin_manifest_store_impl_->Add(
      om.Clone(), base::BindOnce(std::move(callback), std::move(om)));
}

void OriginManifestStoreClient::GetCORSPreflight(
    const std::string& origin,
    GetCORSPreflightCallback callback) {
  origin_manifest_store_impl_->GetCORSPreflight(origin, std::move(callback));
}

void OriginManifestStoreClient::GetContentSecurityPolicies(
    const std::string& origin,
    base::OnceCallback<void(std::vector<mojom::ContentSecurityPolicyPtr>)>
        callback) {
  origin_manifest_store_impl_->GetContentSecurityPolicies(origin,
                                                          std::move(callback));
}

const char* OriginManifestStoreClient::GetNameForLogging() {
  return "OriginManifestStoreClient";
}

}  // namespace origin_manifest
