// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/push_messaging/push_messaging_client.h"

#include "content/public/common/manifest.h"
#include "third_party/WebKit/public/platform/WebString.h"
#include "third_party/WebKit/public/platform/modules/manifest/manifest_manager.mojom.h"
#include "url/gurl.h"

namespace content {

PushMessagingClient::PushMessagingClient(
    blink::mojom::ManifestManager* manifest_manager)
    : manifest_manager_(manifest_manager) {}

PushMessagingClient::~PushMessagingClient() = default;

void PushMessagingClient::GetApplicationServerKeyFromManifest(
    base::OnceCallback<void(const blink::WebString&)> callback) {
  // base::Unretained() is safe because the lifetime of the manifest manager is
  // identical to the lifetime of |this|.
  manifest_manager_->RequestManifest(
      base::BindOnce(&PushMessagingClient::GotApplicationServerKeyFromManifest,
                     base::Unretained(this), std::move(callback)));
}

void PushMessagingClient::GotApplicationServerKeyFromManifest(
    base::OnceCallback<void(const blink::WebString&)> callback,
    const GURL& url,
    const Manifest& manifest) {
  std::move(callback).Run(blink::WebString::FromUTF16(manifest.gcm_sender_id));
}

}  // namespace content
