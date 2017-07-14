// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MediaPlayerClient_h
#define MediaPlayerClient_h

#include "core/CoreExport.h"

namespace blink {

class WebMediaPlayerSource;
class WebMediaPlayerClient;

class CORE_EXPORT MediaPlayerClient {
 public:
  virtual ~MediaPlayerClient() {}

  virtual std::unique_ptr<WebMediaPlayer> CreateWebMediaPlayer(
      const WebMediaPlayerSource&,
      WebMediaPlayerClient*) = 0;
};

}  // namespace blink

#endif  // MediaPlayerClient_h
