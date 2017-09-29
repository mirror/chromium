// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/origin_manifest/origin_manifest_store_client_host.h"

#include "base/bind.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "content/browser/origin_manifest/origin_manifest_fetcher.h"
#include "content/browser/origin_manifest/origin_manifest_store_impl.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace content {

OriginManifestStoreClientHost::OriginManifestStoreClientHost(
    OriginManifestStoreImpl* store,
    net::URLRequestContextGetter* getter)
    : origin_manifest_store_impl_(store), getter_(getter) {}

OriginManifestStoreClientHost::~OriginManifestStoreClientHost() {}

void OriginManifestStoreClientHost::Get(const std::string& origin,
                                        GetCallback callback) {
  std::move(callback).Run(
      origin_manifest_store_impl_->Get(url::Origin(GURL(origin))));
}

void OriginManifestStoreClientHost::GetOrFetch(const std::string& origin,
                                               const std::string& version,
                                               GetCallback callback) {
  mojom::OriginManifestPtr om =
      origin_manifest_store_impl_->Get(url::Origin(GURL(origin)));

  if (getter_ && (om.Equals(nullptr) || (om->version != version))) {
    // TODO(dhausknecht): I am not so sure about this dangling fetcher. It
    // cleans up itself on OnFetchComplete. But if the client gets destroyed
    // while it is not complete... and the client is managed by the
    // StringBinding.
    OriginManifestFetcher* fetcher =
        new OriginManifestFetcher(origin, version, getter_);
    fetcher->Fetch(
        base::BindOnce(&OriginManifestStoreClientHost::OnFetchComplete,
                       base::Unretained(this), std::move(callback)));
    return;
  }

  std::move(callback).Run(std::move(om));
}

void OriginManifestStoreClientHost::OnFetchComplete(
    GetCallback callback,
    mojom::OriginManifestPtr om) {
  if (om.Equals(nullptr)) {
    std::move(callback).Run(std::move(om));
    return;
  }

  origin_manifest_store_impl_->Add(
      om.Clone(), base::BindOnce(std::move(callback), std::move(om)));
}

void OriginManifestStoreClientHost::Remove(const std::string& origin,
                                           RemoveCallback callback) {
  origin_manifest_store_impl_->Remove(url::Origin(GURL(origin)),
                                      std::move(callback));
}

void OriginManifestStoreClientHost::GetCORSPreflight(
    const std::string& origin,
    GetCORSPreflightCallback callback) {
  origin_manifest_store_impl_->GetCORSPreflight(url::Origin(GURL(origin)),
                                                std::move(callback));
}

void OriginManifestStoreClientHost::GetContentSecurityPolicies(
    const std::string& origin,
    base::OnceCallback<void(std::vector<mojom::ContentSecurityPolicyPtr>)>
        callback) {
  origin_manifest_store_impl_->GetContentSecurityPolicies(
      url::Origin(GURL(origin)), std::move(callback));
}

const char* OriginManifestStoreClientHost::GetNameForLogging() {
  return "OriginManifestStoreClientHost";
}

}  // namespace content
