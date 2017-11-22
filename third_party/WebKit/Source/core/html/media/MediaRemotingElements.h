// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MediaRemotingElements_h
#define MediaRemotingElements_h

#include "core/html/media/MediaRemotingInterstitial.h"

namespace blink {

// A text message shown to indicate media remoting is ongoing.
class MediaRemotingCastMessageElement final : public HTMLDivElement {
 public:
  explicit MediaRemotingCastMessageElement(MediaRemotingInterstitial&);
};

// An icon shown to indicate media remoting is ongoing.
class MediaRemotingCastIconElement final : public HTMLDivElement {
 public:
  explicit MediaRemotingCastIconElement(MediaRemotingInterstitial&);
};

}  // namespace blink

#endif  // MediaRemotingElements_h
