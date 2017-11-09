// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/installedapp/related_apps_fetcher.h"

#include "base/bind.h"
#include "content/public/common/manifest.h"
#include "third_party/WebKit/public/platform/WebString.h"
#include "third_party/WebKit/public/platform/modules/installedapp/WebRelatedApplication.h"
#include "third_party/WebKit/public/platform/modules/manifest/manifest.mojom.h"

namespace content {

RelatedAppsFetcher::RelatedAppsFetcher(
    blink::mojom::ManifestManager* manifest_manager)
    : manifest_manager_(manifest_manager) {}

RelatedAppsFetcher::~RelatedAppsFetcher() {}

void RelatedAppsFetcher::GetManifestRelatedApplications(
    std::unique_ptr<blink::WebCallbacks<
        const blink::WebVector<blink::WebRelatedApplication>&,
        void>> callbacks) {
  manifest_manager_->RequestManifest(
      base::BindOnce(&RelatedAppsFetcher::OnGetManifestForRelatedApplications,
                     base::Unretained(this), base::Passed(&callbacks)));
}

void RelatedAppsFetcher::OnGetManifestForRelatedApplications(
    std::unique_ptr<blink::WebCallbacks<
        const blink::WebVector<blink::WebRelatedApplication>&,
        void>> callbacks,
    const GURL& /*url*/,
    const base::Optional<Manifest>& manifest,
    blink::mojom::ManifestDebugInfoPtr debug_info) {
  std::vector<blink::WebRelatedApplication> related_apps;
  if (manifest) {
    for (const auto& related_application : manifest->related_applications) {
      blink::WebRelatedApplication web_related_application;
      web_related_application.platform =
          blink::WebString::FromUTF16(related_application.platform);
      web_related_application.id =
          blink::WebString::FromUTF16(related_application.id);
      if (!related_application.url.is_empty()) {
        web_related_application.url =
            blink::WebString::FromUTF8(related_application.url.spec());
      }
      related_apps.push_back(std::move(web_related_application));
    }
  }
  callbacks->OnSuccess(std::move(related_apps));
}

}  // namespace content
