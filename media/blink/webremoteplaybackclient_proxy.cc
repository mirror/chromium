// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/blink/webremoteplaybackclient_proxy.h"

#include "third_party/WebKit/public/platform/WebURL.h"
#include "third_party/WebKit/public/platform/modules/remoteplayback/WebRemotePlaybackClient.h"

namespace media {

WebRemotePlaybackClientProxy::WebRemotePlaybackClientProxy(
    blink::WebRemotePlaybackClient* client)
    : client_(client) {
  DCHECK(client_);
}

void WebRemotePlaybackClientProxy::SourceChanged(const GURL& url,
                                                 bool is_compatible) {
  client_->SourceChanged(blink::WebURL(url), is_compatible);
}

}  // namespace media