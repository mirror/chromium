// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_INSTALLEDAPP_RELATED_APPS_FETCHER_H
#define CONTENT_RENDERER_INSTALLEDAPP_RELATED_APPS_FETCHER_H

#include <string>

#include "base/compiler_specific.h"
#include "content/common/content_export.h"
#include "third_party/WebKit/public/platform/modules/installedapp/WebRelatedAppsFetcher.h"

namespace content {

class RenderFrameImpl;
struct Manifest;

class CONTENT_EXPORT RelatedAppsFetcher : public blink::WebRelatedAppsFetcher {
 public:
  explicit RelatedAppsFetcher(RenderFrameImpl* frame);
  ~RelatedAppsFetcher() override;

  // blink::WebRelatedAppsFetcher overrides:
  void GetManifestRelatedApplications(
      std::unique_ptr<blink::WebCallbacks<
          const blink::WebVector<blink::WebRelatedApplication>&,
          void>> callbacks) override;

 private:
  // Callback for when the manifest has been fetched, triggered by a call to
  // getManifestRelatedApplications.
  void OnGetManifestForRelatedApplications(
      std::unique_ptr<blink::WebCallbacks<
          const blink::WebVector<blink::WebRelatedApplication>&,
          void>> callbacks,
      const GURL& url,
      const Manifest& manifest);

  RenderFrameImpl* const frame_;

  DISALLOW_COPY_AND_ASSIGN(RelatedAppsFetcher);
};

}  // namespace content

#endif  // CONTENT_RENDERER_INSTALLEDAPP_RELATED_APPS_FETCHER_H
