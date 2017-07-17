// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_BLINK_WEBREMOTEPLAYBACKCLIENT_PROXY_H_
#define MEDIA_BLINK_WEBREMOTEPLAYBACKCLIENT_PROXY_H_

#include <stddef.h>

#include <string>

#include "base/macros.h"
#include "media/blink/media_blink_export.h"

class GURL;

namespace blink {
class WebRemotePlaybackClient;
}

namespace media {

// A proxy that allows media/remoting/ code to use
// blink::WebRemotePlaybackClient without a layering violation (blink:: can only
// be used directly from media/blink/). It doesn't own the
// WebRemotePlaybackClient object so MUST not outlive it.
class MEDIA_BLINK_EXPORT WebRemotePlaybackClientProxy final {
 public:
  explicit WebRemotePlaybackClientProxy(blink::WebRemotePlaybackClient* client);

  void SourceChanged(const GURL& url, bool is_compatible);

 private:
  blink::WebRemotePlaybackClient* client_;

  DISALLOW_COPY_AND_ASSIGN(WebRemotePlaybackClientProxy);
};

}  // namespace media

#endif  // MEDIA_BLINK_WEBREMOTEPLAYBACKCLIENT_PROXY_H_
