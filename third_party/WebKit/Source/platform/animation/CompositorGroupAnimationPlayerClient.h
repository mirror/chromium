// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CompositorGroupAnimationPlayerClient_h
#define CompositorGroupAnimationPlayerClient_h

#include "public/platform/WebCommon.h"

namespace blink {

class CompositorGroupAnimationPlayer;

// A client for compositor representation of GroupAnimationPlayer.
class BLINK_PLATFORM_EXPORT CompositorGroupAnimationPlayerClient {
 public:
  virtual ~CompositorGroupAnimationPlayerClient();

  virtual CompositorGroupAnimationPlayer* CompositorGroupPlayer() const = 0;
};

}  // namespace blink

#endif  // CompositorGroupAnimationPlayerClient_h
