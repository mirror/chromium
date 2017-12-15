// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef AnimationWorkletProxyClientImpl_h
#define AnimationWorkletProxyClientImpl_h

#include "core/animation/CompositorAnimator.h"
#include "core/animation/CompositorMutatorImpl.h"
#include "core/dom/AnimationWorkletProxyClient.h"
#include "modules/ModulesExport.h"
#include "modules/animationworklet/AnimationWorkletGlobalScope.h"
#include "platform/CrossThreadFunctional.h"
#include "platform/heap/Handle.h"
#include "platform/wtf/Noncopyable.h"
#include "platform/WaitableEvent.h"

namespace blink {

class CompositorMutatorImpl;
class Document;
class WorkletGlobalScope;

// Mediates between one Animator and the associated CompositorMutatorImpl. There
// is one AnimationWorkletProxyClientImpl per Animator but there may be multiple
// for a given mutator and animatorWorklet.
//
// This is constructed on the main thread but it is used in the worklet backing
// thread.
class MODULES_EXPORT AnimationWorkletProxyClientImpl final
    : public GarbageCollectedFinalized<AnimationWorkletProxyClientImpl>,
      public AnimationWorkletProxyClient,
      public CompositorAnimator {
  WTF_MAKE_NONCOPYABLE(AnimationWorkletProxyClientImpl);
  USING_GARBAGE_COLLECTED_MIXIN(AnimationWorkletProxyClientImpl);

 public:
  explicit AnimationWorkletProxyClientImpl(CompositorMutatorImpl*);
  void Trace(blink::Visitor*) override;

  // AnimationWorkletProxyClient:
  void SetGlobalScope(WorkletGlobalScope*) override;
  void Dispose() override;

  // CompositorAnimator:
  // This method is invoked in compositor thread
  void Mutate(const CompositorMutatorInputState&) override;

  static AnimationWorkletProxyClientImpl* FromDocument(Document*);

 private:
  // This method is invoked in the worklet thread
  void MutateWithEvent(const CompositorMutatorInputState*,
                       std::unique_ptr<CompositorMutatorOutputState>*);

  CompositorMutatorImpl* mutator_;

  CrossThreadWeakPersistent<AnimationWorkletGlobalScope> global_scope_;
  bool active_;
  WaitableEvent is_done_;
};

}  // namespace blink

#endif  // AnimationWorkletProxyClientImpl_h
