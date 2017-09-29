// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef AnimationWorkletGlobalScope_h
#define AnimationWorkletGlobalScope_h

#include "bindings/core/v8/ScriptValue.h"
#include "core/workers/ThreadedWorkletGlobalScope.h"
#include "modules/ModulesExport.h"
#include "modules/compositorworker/Animator.h"
#include "modules/compositorworker/AnimatorDefinition.h"
#include "platform/bindings/ScriptWrappable.h"
#include "platform/graphics/CompositorAnimatorsState.h"

namespace blink {

class ExceptionState;
class WorkerClients;

class MODULES_EXPORT AnimationWorkletGlobalScope
    : public ThreadedWorkletGlobalScope {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static AnimationWorkletGlobalScope* Create(const KURL&,
                                             const String& user_agent,
                                             RefPtr<SecurityOrigin>,
                                             v8::Isolate*,
                                             WorkerThread*,
                                             WorkerClients*);
  ~AnimationWorkletGlobalScope() override;
  DECLARE_TRACE();
  DECLARE_TRACE_WRAPPERS();
  void Dispose() override;
  bool IsAnimationWorkletGlobalScope() const final { return true; }

  Animator* CreateInstance(const String& name);

  std::unique_ptr<CompositorMutatorOutputState> Mutate(
      const CompositorMutatorInputState&);

  void registerAnimator(const String& name,
                        const ScriptValue& ctorValue,
                        ExceptionState&);

  void createAnimatorForTest(int player_id, const String& name);

  AnimatorDefinition* FindDefinitionForTest(const String& name);
  unsigned GetAnimatorsSizeForTest() { return animators_.size(); }

 private:
  AnimationWorkletGlobalScope(const KURL&,
                              const String& user_agent,
                              RefPtr<SecurityOrigin>,
                              v8::Isolate*,
                              WorkerThread*,
                              WorkerClients*);

  Animator* EnsureAnimator(int player_id, const String& name);
  typedef HeapHashMap<String, TraceWrapperMember<AnimatorDefinition>>
      DefinitionMap;
  DefinitionMap animator_definitions_;

  typedef HeapHashMap<int, TraceWrapperMember<Animator>> AnimatorMap;
  AnimatorMap animators_;
};

}  // namespace blink

#endif  // AnimationWorkletGlobalScope_h
