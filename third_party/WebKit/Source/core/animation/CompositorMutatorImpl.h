// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CompositorMutatorImpl_h
#define CompositorMutatorImpl_h

#include <algorithm>
#include <memory>
#include <vector>

#include "base/macros.h"
#include "core/animation/CompositorAnimator.h"
#include "platform/graphics/CompositorMutator.h"
#include "platform/heap/Handle.h"
#include "platform/heap/HeapAllocator.h"

namespace blink {

class CompositorMutatorClient;
class WaitableEvent;

// Fans out requests from the compositor to all of the registered
// CompositorAnimators which can then mutate layers through their respective
// mutate interface. Requests for animation frames are received from
// CompositorAnimators and sent to the compositor to generate a new compositor
// frame.
//
// Unless otherwise noted, this should be accessed only on the compositor
// thread.
class CORE_EXPORT CompositorMutatorImpl final : public CompositorMutator {
 public:
  static std::unique_ptr<CompositorMutatorImpl> Create();
  std::unique_ptr<CompositorMutatorClient> CreateClient();

  // CompositorMutator implementation.
  void Mutate(std::unique_ptr<CompositorMutatorInputState>) override;
  // TODO(majidvp): Remove when timeline inputs are known.
  bool HasAnimators() override { return has_animators_; }

  // Interface for use by the AnimationWorklet Thread(s)
  void RegisterCompositorAnimator(CompositorAnimator*);
  void UnregisterCompositorAnimator(CompositorAnimator*);
  void SetMutationUpdate(std::unique_ptr<CompositorMutatorOutputState>);

  void SetClient(CompositorMutatorClient*);

 private:
  CompositorMutatorImpl();

  void RegisterCompositorAnimatorInternal(CompositorAnimator*);
  void UnregisterCompositorAnimatorInternal(CompositorAnimator*);

  void MutateWithEvent(const CompositorMutatorInputState*,
                       CompositorMutatorOutputState*,
                       WaitableEvent*);

  using CompositorAnimators =
      HashSet<CrossThreadPersistent<CompositorAnimator>>;
  CompositorAnimators animators_;
  bool has_animators_;

  CompositorMutatorClient* client_;
  std::vector<std::unique_ptr<CompositorMutatorOutputState>> outputs_;

  DISALLOW_COPY_AND_ASSIGN(CompositorMutatorImpl);
};

}  // namespace blink

#endif  // CompositorMutatorImpl_h
