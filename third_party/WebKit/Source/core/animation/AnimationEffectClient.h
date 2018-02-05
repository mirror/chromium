// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef AnimationEffectClient_h
#define AnimationEffectClient_h

#include "platform/heap/GarbageCollected.h"

namespace blink {

class Animation;

// The client interface is used by AnimationEffect to interact with its
// owning animation.
class AnimationEffectClient : public GarbageCollectedMixin {
 public:
  AnimationEffectClient() = default;

  virtual unsigned SequenceNumber() const = 0;
  virtual bool Playing() const = 0;
  // TODO(majidvp): Check if Paused can be replaced with !Playing()
  virtual bool Paused() const = 0;
  virtual bool HasStartTime() const = 0;
  virtual bool EffectSuppressed() const = 0;

  virtual void SpecifiedTimingChanged() = 0;
  virtual void UpdateIfNecessary() = 0;

  // TODO(majidvp): Remove this. Exposing the animation instance here is not
  // ideal as it punches a hole in our abstraction. This is currently necessary
  // as CompositorAnimations and EffectStack need to access the animation
  // instance but we should try to remove this.
  virtual Animation* GetAnimation() = 0;

 protected:
  virtual ~AnimationEffectClient() = default;
};

}  // namespace blink

#endif  // AnimationEffectClient_h
