// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_PUSH_MESSAGING_PUSH_MESSAGING_CLIENT_H_
#define CONTENT_RENDERER_PUSH_MESSAGING_PUSH_MESSAGING_CLIENT_H_

#include "base/callback_forward.h"
#include "base/macros.h"
#include "third_party/WebKit/public/platform/modules/manifest/manifest_manager.mojom.h"
#include "third_party/WebKit/public/platform/modules/push_messaging/WebPushClient.h"

namespace blink {
class WebString;
}

class GURL;

namespace content {

struct Manifest;

class PushMessagingClient : public blink::WebPushClient {
 public:
  explicit PushMessagingClient(blink::mojom::ManifestManager* manifest_manager);
  ~PushMessagingClient() override;

  // blink::WebPushClient implementation:
  void GetApplicationServerKeyFromManifest(
      base::OnceCallback<void(const blink::WebString&)> callback) override;

 private:
  // Called when the manifest, if any, has been loaded. Failures in loading
  // the manifest will be communicated through an empty origin.
  void GotApplicationServerKeyFromManifest(
      base::OnceCallback<void(const blink::WebString&)> callback,
      const GURL& url,
      const Manifest& manifest);

  // The manifest manager associated with the same RenderFrameImpl as |this|.
  blink::mojom::ManifestManager* manifest_manager_;

  DISALLOW_COPY_AND_ASSIGN(PushMessagingClient);
};

}  // namespace content

#endif  // CONTENT_RENDERER_PUSH_MESSAGING_PUSH_MESSAGING_CLIENT_H_
