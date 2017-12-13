// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebPushClient_h
#define WebPushClient_h

#include "base/callback_forward.h"

namespace blink {

class WebString;

class WebPushClient {
 public:
  virtual ~WebPushClient() = default;

  // Loads the manifest associated with the current frame and, if any, invokes
  // the |callback| with the application server key defined within it.
  virtual void GetApplicationServerKeyFromManifest(
      base::OnceCallback<void(const WebString&)>) = 0;
};

}  // namespace blink

#endif  // WebPushClient_h
