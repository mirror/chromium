// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef Animator_h
#define Animator_h

#include "modules/compositorworker/EffectProxy.h"
#include "platform/bindings/ScriptWrappable.h"
#include "platform/bindings/TraceWrapperMember.h"
#include "platform/bindings/TraceWrapperV8Reference.h"
#include "platform/graphics/CompositorAnimatorsState.h"
#include "platform/heap/Handle.h"
#include "platform/wtf/Time.h"
#include "v8/include/v8.h"

namespace blink {

class AnimatorDefinition;
class ScriptState;

class Animator final : public GarbageCollectedFinalized<Animator>,
                       public TraceWrapperBase {
 public:
  Animator(v8::Isolate*, AnimatorDefinition*, v8::Local<v8::Object> instance);
  ~Animator();
  DECLARE_TRACE();
  DECLARE_TRACE_WRAPPERS();

  // Updates the animator with latest state coming from |AnimationHost|.
  void PushInputState(const CompositorMutatorInputState::AnimationState&);

  // Returns true if it successfully invoked animate callback in JS and fills
  // the output state with new state.
  bool Animate(ScriptState*,
               CompositorMutatorOutputState::AnimationState*) const;

 private:
  // This object keeps the definition object, and animator instance alive.
  // It participates in wrapper tracing as it holds onto V8 wrappers.
  TraceWrapperMember<AnimatorDefinition> definition_;
  TraceWrapperV8Reference<v8::Object> instance_;

  WTF::TimeTicks current_time_;
  Member<EffectProxy> effect_;
};

}  // namespace blink

#endif  // Animator_h
