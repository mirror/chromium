// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebMediaPlayerFactory_h
#define WebMediaPlayerFactory_h

#include <memory>

#include "public/platform/WebCommon.h"

namespace blink {

class WebContentDecryptionModule;
class WebFetchContext;
class WebMediaPlayer;
class WebMediaPlayerClient;
class WebMediaPlayerEncryptedMediaClient;
class WebMediaPlayerSource;
class WebString;

class BLINK_PLATFORM_EXPORT WebMediaPlayerFactory {
 public:
  virtual ~WebMediaPlayerFactory() {}

  virtual void Initialize(blink::WebFetchContext*) = 0;

  virtual std::unique_ptr<WebMediaPlayer> CreateMediaPlayer(
      const WebMediaPlayerSource&,
      WebMediaPlayerClient*,
      WebMediaPlayerEncryptedMediaClient*,
      WebContentDecryptionModule*,
      const WebString& sink_id) = 0;
};

}  // namespace blink

#endif  // WebMediaPlayerFactory_h
