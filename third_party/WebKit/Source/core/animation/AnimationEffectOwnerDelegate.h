// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef AnimationEffectOwnerDelegate_h
#define AnimationEffectOwnerDelegate_h

#include "core/animation/AnimationEffectReadOnly.h"
#include "platform/heap/GarbageCollected.h"

namespace blink {
class AnimationEffectOwnerDelegate
    : public GarbageCollectedFinalized<AnimationEffectOwnerDelegate> {
 public:
  // TODO(smcgruer): These would all be = 0 in the real approach.
  virtual void SetOutdated() {}
  virtual bool EffectSuppressed() const { return false; }
  virtual bool HasStartTime() const { return false; }
  virtual bool Playing() const { return false; }
  virtual bool Paused() const { return false; }
  virtual bool Outdated() { return false; }
  virtual bool Update(TimingUpdateReason) { return false; }
  virtual void cancel() {}
  virtual void setEffect(AnimationEffectReadOnly*) {}
  virtual unsigned SequenceNumber() const { return 0; }

  DEFINE_INLINE_VIRTUAL_TRACE() {}
};
}  // namespace blink

#endif  // AnimationEffectOwnerDelegate_h
