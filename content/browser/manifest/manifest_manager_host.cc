// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/manifest/manifest_manager_host.h"

#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/manifest.h"
#include "services/service_manager/public/cpp/interface_provider.h"

namespace content {

ManifestManagerHost::ManifestManagerHost(WebContents* web_contents)
    : WebContentsObserver(web_contents) {}

ManifestManagerHost::~ManifestManagerHost() = default;

void ManifestManagerHost::RenderFrameDeleted(
    RenderFrameHost* render_frame_host) {
  if (render_frame_host == manifest_manager_frame_)
    OnConnectionError();
}

void ManifestManagerHost::GetManifest(const GetManifestCallback& callback) {
  int request_id = callbacks_.Add(new GetManifestCallback(callback));

  GetManifestManager().RequestManifest(
      base::Bind(&ManifestManagerHost::OnRequestManifestResponse,
                 base::Unretained(this), request_id));
}

blink::mojom::ManifestManager& ManifestManagerHost::GetManifestManager() {
  if (manifest_manager_frame_ != web_contents()->GetMainFrame())
    OnConnectionError();

  if (!manifest_manager_) {
    manifest_manager_frame_ = web_contents()->GetMainFrame();
    manifest_manager_frame_->GetRemoteInterfaces()->GetInterface(
        &manifest_manager_);
    manifest_manager_.set_connection_error_handler(base::Bind(
        &ManifestManagerHost::OnConnectionError, base::Unretained(this)));
  }
  return *manifest_manager_;
}

void ManifestManagerHost::OnConnectionError() {
  manifest_manager_frame_ = nullptr;
  manifest_manager_.reset();
  for (CallbackMap::const_iterator it(&callbacks_); !it.IsAtEnd();
       it.Advance()) {
    it.GetCurrentValue()->Run(GURL(), Manifest());
  }
  callbacks_.Clear();
}

void ManifestManagerHost::OnRequestManifestResponse(
    int request_id,
    const GURL& url,
    const base::Optional<Manifest>& manifest) {
  auto* callback = callbacks_.Lookup(request_id);
  if (manifest)
    callback->Run(url, *manifest);
  else
    callback->Run(url, Manifest());
  callbacks_.Remove(request_id);
}

}  // namespace content
