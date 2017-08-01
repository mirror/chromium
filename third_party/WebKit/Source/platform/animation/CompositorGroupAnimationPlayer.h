// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CompositorGroupAnimationPlayer_h
#define CompositorGroupAnimationPlayer_h

#include <memory>
#include "base/memory/ref_counted.h"
#include "cc/animation/animation_delegate.h"
#include "cc/animation/animation_player.h"
#include "cc/animation/group_animation_player.h"
#include "platform/PlatformExport.h"
//#include "platform/graphics/CompositorElementId.h"
#include "platform/wtf/Noncopyable.h"
#include "platform/wtf/PtrUtil.h"

namespace cc {
class AnimationCurve;
}

namespace blink {

class CompositorAnimation;
class CompositorAnimationDelegate;
class CompositorAnimationPlayer;

// A compositor representation for AnimationPlayer.
class PLATFORM_EXPORT CompositorGroupAnimationPlayer
    : public cc::AnimationDelegate {
  WTF_MAKE_NONCOPYABLE(CompositorGroupAnimationPlayer);

 public:
  static std::unique_ptr<CompositorGroupAnimationPlayer> Create() {
    return WTF::WrapUnique(new CompositorGroupAnimationPlayer());
  }

  ~CompositorGroupAnimationPlayer();

  cc::GroupAnimationPlayer* CcGroupAnimationPlayer() const;

  // An animation delegate is notified when animations are started and stopped.
  // The CompositorAnimationPlayer does not take ownership of the delegate, and
  // it is the responsibility of the client to reset the layer's delegate before
  // deleting the delegate.
  void SetAnimationDelegate(CompositorAnimationDelegate*);

  //  void AttachElement(const CompositorElementId&);
  //  void DetachElement();
  //  bool IsElementAttached() const;
  //
  //  void AddAnimation(std::unique_ptr<CompositorAnimation>);
  //  void RemoveAnimation(int animation_id);
  //  void PauseAnimation(int animation_id, double time_offset);
  //  void AbortAnimation(int animation_id);

 private:
  CompositorGroupAnimationPlayer();

  // cc::AnimationDelegate implementation.
  void NotifyAnimationStarted(base::TimeTicks monotonic_time,
                              int target_property,
                              int group) override;
  void NotifyAnimationFinished(base::TimeTicks monotonic_time,
                               int target_property,
                               int group) override;
  void NotifyAnimationAborted(base::TimeTicks monotonic_time,
                              int target_property,
                              int group) override;
  void NotifyAnimationTakeover(base::TimeTicks monotonic_time,
                               int target_property,
                               base::TimeTicks animation_start_time,
                               std::unique_ptr<cc::AnimationCurve>) override;

  scoped_refptr<cc::GroupAnimationPlayer> group_animation_player_;
  CompositorAnimationDelegate* delegate_;
};

}  // namespace blink

#endif  // CompositorAnimationPlayer_h
